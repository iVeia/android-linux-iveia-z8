/*
 * iVeia passthrough driver
 *
 * Copyright iVeia 2021
 *
 * Contact: Maxwell Walter (mwalter@iveia.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define DEBUG 1

#include <linux/device.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#include <media/v4l2-async.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-subdev.h>

#define IVDS_WIDTH_MIN 640  
#define IVDS_WIDTH_MAX 10328
#define IVDS_HEIGHT_MIN 480 
#define IVDS_HEIGHT_MAX 7760


struct ivds_info {
  struct device *dev;
  struct v4l2_subdev subdev;
  
  // Currently one in / two out
  struct media_pad pads[1];

  struct v4l2_mbus_framefmt formats[1];
  struct v4l2_mbus_framefmt default_format;

  bool streaming;
};


static inline struct ivds_info *get_info_from_subdev(struct v4l2_subdev *subdev) {
  return container_of(subdev, struct ivds_info, subdev);
}

static struct v4l2_subdev_core_ops ivds_cops = {
  //None
};

static int ivds_s_stream(struct v4l2_subdev *subdev, int enable) {
  struct ivds_info *info = get_info_from_subdev(subdev);
  dev_dbg(info->dev, "Stream on\n");
  
  // Just return true always
  return 0;
}

static struct v4l2_subdev_video_ops ivds_vops = {
  .s_stream = ivds_s_stream,
};


static int ivds_enum_mbus_code(struct v4l2_subdev *subdev,
                               struct v4l2_subdev_pad_config *cfg,
                               struct v4l2_subdev_mbus_code_enum *code) {
  struct v4l2_mbus_framefmt *format;
  
  if (code->which == V4L2_SUBDEV_FORMAT_ACTIVE) {
    pr_warn("ivds: Which is active: %d\n", code->which);
    return -EINVAL;
  }
  
  if (code->index) {
    pr_warn("ivds: index is %d\n", code->index);
    return -EINVAL;
  }

  // My try_format just passes the format through, so this is basically just
  //  saying "yes" to every format that gets passed in
  format = v4l2_subdev_get_try_format(subdev, cfg, code->pad);
  code->code = format->code;
  
  return 0;
}
static int ivds_enum_frame_size(struct v4l2_subdev *subdev,
                                  struct v4l2_subdev_pad_config *config,
                                  struct v4l2_subdev_frame_size_enum *fsize) {
  // Hard coded for now - limit to the max size of Xilinx framebuffer
  fsize->min_width =  IVDS_WIDTH_MIN;
  fsize->max_width =  IVDS_WIDTH_MAX;
  fsize->min_height = IVDS_HEIGHT_MIN;
  fsize->max_height = IVDS_HEIGHT_MAX;
    
  return 0;
}

static int ivds_get_fmt(struct v4l2_subdev *subdev,
                          struct v4l2_subdev_pad_config *cfg,
                          struct v4l2_subdev_format *fmt) {
  struct ivds_info *info = get_info_from_subdev(subdev);
  
  switch (fmt->which) {
  case V4L2_SUBDEV_FORMAT_TRY:
    fmt->format =  *v4l2_subdev_get_try_format(subdev, cfg, fmt->pad);
    break;
  case V4L2_SUBDEV_FORMAT_ACTIVE:
    fmt->format =  info->formats[fmt->pad];
    break;
  default:
    break;
  }
  dev_dbg(info->dev, "** Trying to get format to (%d x %d) %d %d %d %d %d :: %d\n",
          fmt->format.width,
          fmt->format.height,
          fmt->format.code,
          fmt->format.field,
          fmt->format.colorspace,
          fmt->format.ycbcr_enc,
          fmt->format.quantization,
          fmt->which);
  
  return 0;
}

static int ivds_set_fmt(struct v4l2_subdev *subdev,
                          struct v4l2_subdev_pad_config *cfg,
                          struct v4l2_subdev_format *fmt) {
  struct ivds_info *info = get_info_from_subdev(subdev);
  
  clamp_t(unsigned int, fmt->format.width, IVDS_WIDTH_MIN, IVDS_WIDTH_MAX);
  clamp_t(unsigned int, fmt->format.height, IVDS_HEIGHT_MIN, IVDS_HEIGHT_MAX);
  
  dev_dbg(info->dev, "** Trying to set format to (%d x %d) %d %d %d %d %d (pad:%d):: %d\n",
          fmt->format.width,
          fmt->format.height,
          fmt->format.code,
          fmt->format.field,
          fmt->format.colorspace,
          fmt->format.ycbcr_enc,
          fmt->format.quantization,
          fmt->pad,
          fmt->which);


  switch(fmt->which) {
  case V4L2_SUBDEV_FORMAT_TRY:
    *v4l2_subdev_get_try_format(subdev, cfg, fmt->pad) = fmt->format;
    break;
  case V4L2_SUBDEV_FORMAT_ACTIVE:
    info->formats[fmt->pad] = fmt->format;
    break;
  default:
    return -EINVAL;
  }

  return 0;
}

static struct v4l2_subdev_pad_ops ivds_pops = {
  .enum_mbus_code       = ivds_enum_mbus_code,
  .enum_frame_size      = ivds_enum_frame_size,
  .get_fmt              = ivds_get_fmt,
  .set_fmt              = ivds_set_fmt,
};


static struct v4l2_subdev_ops ivds_ops = {
  .core  = &ivds_cops,
  .video = &ivds_vops,
  .pad   = &ivds_pops,
};

static int ivds_open(struct v4l2_subdev *subdev, struct v4l2_subdev_fh *fh) {
  struct ivds_info *info = get_info_from_subdev(subdev);
  struct v4l2_mbus_framefmt *format;

  dev_dbg(info->dev, "open\n");

  // Set to the default format on open
  format = v4l2_subdev_get_try_format(subdev, fh->pad, 0);
  *format = info->default_format;
  
  return 0;
}
static int ivds_close(struct v4l2_subdev *subdev, struct v4l2_subdev_fh *fh) {
  return 0;
}
static const struct v4l2_subdev_internal_ops ivds_internal_ops = {
  .open = ivds_open,
  .close = ivds_close,
};

static const struct media_entity_operations ivds_media_ops = {
  .link_validate = v4l2_subdev_link_validate,
};


static int ivds_probe(struct platform_device *pdev) {
  struct ivds_info *info;
  struct v4l2_subdev *subdev;
  u32 ret;

  info = devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
  if(!info) {
    return -ENOMEM;
  }
  info->dev = &pdev->dev;

  dev_warn(&pdev->dev, "probing iVeia passthrough driver\n");

  // Use Xilinx' defaults here (just because)
  info->default_format.code = MEDIA_BUS_FMT_VYYUYY8_1X24;
  info->default_format.field = V4L2_FIELD_NONE;
  info->default_format.colorspace = V4L2_COLORSPACE_REC709;

  // Currently, pad0 is input, pad1 though padN is output
  info->pads[0].flags = MEDIA_PAD_FL_SOURCE;
  info->formats[0] = info->default_format;

  subdev = &info->subdev;
  v4l2_subdev_init(subdev, &ivds_ops);
  subdev->dev = info->dev;
  subdev->internal_ops = &ivds_internal_ops;
  strlcpy(subdev->name, dev_name(&pdev->dev), sizeof(subdev->name));
  v4l2_set_subdevdata(subdev, info);
  subdev->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
  subdev->entity.ops = &ivds_media_ops;

  ret = media_entity_pads_init(&subdev->entity, 1, info->pads);
  if (ret < 0) {
    dev_dbg(&pdev->dev, "Failed to init media pads\n");
    goto error;
  }

  platform_set_drvdata(pdev, info);

  ret = v4l2_async_register_subdev(subdev);
  if (ret < 0) {
    dev_err(&pdev->dev, "failed to register subdev\n");
    goto error;
  }

  return 0;

error:
  media_entity_cleanup(&subdev->entity);
  return ret;
};

static int ivds_remove(struct platform_device *pdev) {
  struct ivds_info *info = platform_get_drvdata(pdev);
  struct v4l2_subdev *subdev = &info->subdev;
  
  v4l2_async_unregister_subdev(subdev);
  media_entity_cleanup(&subdev->entity);
  
  return 0;
}


static const struct of_device_id ivds_of_table[] = {
  {.compatible = "iveia,dummy-source"},
  {}
};
MODULE_DEVICE_TABLE(of, ivds_of_table);



static int __maybe_unused ivds_pm_suspend(struct device *dev) {
	return 0;
}
static int __maybe_unused ivds_pm_resume(struct device *dev) {
	return 0;
}
static SIMPLE_DEV_PM_OPS(ivds_pm_ops, ivds_pm_suspend, ivds_pm_resume);


static struct platform_driver ivds_driver = {
	.driver = {
		.name		= "iVeia dummy sourcedriver",
		.pm		= &ivds_pm_ops,
		.of_match_table	= ivds_of_table,
	},
	.probe			= ivds_probe,
	.remove			= ivds_remove,
};
module_platform_driver(ivds_driver);

MODULE_AUTHOR("Maxwell Walter <mwalter@iveia.com>");
MODULE_DESCRIPTION("iVeia dummy source driver");
MODULE_LICENSE("GPL v2");

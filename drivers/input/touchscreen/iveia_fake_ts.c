/*
 * Fake touchscreen that will hopefully just generate a /dev/input file
 *
 */
#include <linux/dmi.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/gpio/consumer.h>
#include <asm/unaligned.h>
#include <linux/platform_device.h>

static int iv_fake_probe(struct platform_device *pdev)
{
  struct device *dev = &pdev->dev;
  struct input_dev *input_dev;  

  dev_warn(dev, "Probing fake iVeia input driver");
  
  int error;
  
  int max_x = 640;
  int max_y = 480;
  dev_info(dev, "fake intput size X%uY%u\n", max_x, max_y);
  
  /* Register input device */
  input_dev = input_allocate_device();
  if (!input_dev) {
    dev_err(dev, "Fake device failed to allocate memory\n");
    return -ENOMEM;
  }
  
  input_dev->name = "iVeia Fake input";
  input_dev->dev.parent = dev;
  //input_dev->open = iv_fake_open;
  //input_dev->close = iv_fake_close;
  
  // Say we are a touch screen
  input_set_capability(input_dev, EV_KEY, BTN_TOUCH);
  
  /* For single touch */
  input_set_abs_params(input_dev, ABS_X, 0, max_x, 0, 0);
  input_set_abs_params(input_dev, ABS_Y, 0, max_y, 0, 0);
  
  //	input_set_drvdata(input_dev, data);
  
  error = input_register_device(input_dev);
  if (error) {
    dev_err(dev, "Error %d registering input device\n", error);
    goto err_free_mem;
  }

  dev_warn(dev, "registered input device");
  
  return 0;
 err_free_mem:
  input_free_device(input_dev);
  return error;
}

static int iv_fake_remove(struct platform_device *pdev)
{
  return 0;
}

/*
  static int __maybe_unused ivf_suspend(struct device *dev)
  {
  return 0;
  }
  
  static int __maybe_unused ivf_resume(struct device *dev)
  {
  else return 0;
  }
  
  static SIMPLE_DEV_PM_OPS(ivf_pm_ops, ivf_suspend, ivf_resume);
*/

static struct of_device_id iv_fake_match[] = {
  { .compatible = "iveia,fakets", },
  {},
};

static struct platform_driver iv_fake_driver = {
  .probe = iv_fake_probe,
  .remove = iv_fake_remove,
  .driver = {
    .name = "iVeia Fake Driver",
    .owner = THIS_MODULE,
    .of_match_table = iv_fake_match,
  },
};
module_platform_driver(iv_fake_driver);

/* Module information */
MODULE_AUTHOR("Maxwell Walter <mwalter@iveia.com>");
MODULE_DESCRIPTION("iVeia Fake Touchscreen driver");
MODULE_LICENSE("GPL");

/*

Hacked up version of z7e_wm8960.c to get Audio to work on Atlas-Bone Island

To Playback::::

/etc/iveia/i2crw -d /dev/i2c-0 -a 21 03 fb
amixer sset Headphone,0 100%
amixer sset Playback,0 100% unmute
amixer sset Left\ Output\ Mixer\ PCM,0 on && amixer sset Right\ Output\ Mixer\ PCM,0 on
aplay /usr/share/sounds/alsa/Front_Center.wav 


To Record::::
amixer sset Left\ Boost\ Mixer\ LINPUT1,0 on
amixer sset Left\ Boost\ Mixer\ LINPUT2,0 on
amixer sset Left\ Boost\ Mixer\ LINPUT3,0 off

amixer sset Left\ Input\ Boost\ Mixer\ LINPUT1,0 0
amixer sset Left\ Input\ Mixer\ Boost,0 on

amixer sset Left\ Output\ Mixer\ PCM,0 on
amixer -c 0 cset name='Capture Volume' 100%,100%
amixer -c 0 cset name='Capture Switch' on,on
amixer -c 0 cset name='ADC Data Output Select' 3
arecord -f S16_LE -d 10 audio.wav
 */

#include <linux/module.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include "../codecs/wm8960.h"

static const struct snd_soc_dapm_widget z7_bone_island_wm8960_widgets[] = {
    SND_SOC_DAPM_MIC(    "Mic Jack",            NULL ),
    SND_SOC_DAPM_LINE(    "Line Input 3 (FM)",NULL ),
    SND_SOC_DAPM_HP(    "Headphone Jack",    NULL ),
    SND_SOC_DAPM_SPK(    "Speaker_L",        NULL ),
    SND_SOC_DAPM_SPK(    "Speaker_R",        NULL ),
};
        
static const struct snd_soc_dapm_route z7_bone_island_wm8960_routes[] = {
    { "Headphone Jack",    NULL,    "HP_L"        },
    { "Headphone Jack",    NULL,     "HP_R"        },
    { "Speaker_L",        NULL,     "SPK_LP"    }, 
    { "Speaker_L",        NULL,     "SPK_LN"     }, 
    { "Speaker_R",        NULL,     "SPK_RP"     }, 
    { "Speaker_R",        NULL,     "SPK_RN"     }, 
    { "LINPUT1",        NULL,     "MICB"        },
    { "MICB",            NULL,     "Mic Jack"    },
};

#define AUDIO_FORMAT (SND_SOC_DAIFMT_I2S | \
	SND_SOC_DAIFMT_CBS_CFS | SND_SOC_DAIFMT_NB_NF)

static int z7_bone_island_wm8960_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
    struct snd_soc_pcm_runtime *rtd = substream->private_data;
    struct snd_soc_dai *codec_dai = rtd->codec_dai;
    struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
    struct snd_soc_codec *codec = rtd->codec;
    unsigned int rate = params_rate(params);
    snd_pcm_format_t fmt = params_format( params );
    unsigned sysclk;
    int ret = 0;
 
    int bclk_div;
    int dacdiv;
 
    printk("+%s()\n", __FUNCTION__ );
 
    switch ( fmt ) {
    case SNDRV_PCM_FORMAT_S16:
        bclk_div = 32;
        break;
    case SNDRV_PCM_FORMAT_S20_3LE:
    case SNDRV_PCM_FORMAT_S24:
        bclk_div = 48;
        break;
    default:
        printk("-%s(): PCM FMT ERROR\n", __FUNCTION__ );
        return -EINVAL;
    }
    
    switch ( rate ) {
    case 8018:
        dacdiv = WM8960_DAC_DIV_5_5;
        sysclk = 11289600;
        break;
    case 11025:
        dacdiv = WM8960_DAC_DIV_4;
        sysclk = 11289600;
        break;
    case 22050: 
        dacdiv = WM8960_DAC_DIV_2;
        sysclk = 11289600;
        break;
    case 44100:
        dacdiv = WM8960_DAC_DIV_1;
        sysclk = 11289600;
        break;    
    case 8000:
        dacdiv = WM8960_DAC_DIV_6;
        sysclk = 12288000;
        break;
    case 12000: 
        dacdiv = WM8960_DAC_DIV_4;
        sysclk = 12288000;
        break;
    case 16000: 
        dacdiv = WM8960_DAC_DIV_3;
        sysclk = 12288000;
        break;
    case 24000:
        dacdiv = WM8960_DAC_DIV_2;
        sysclk = 12288000;
        break;
    case 32000:
        dacdiv = WM8960_DAC_DIV_1_5;
        sysclk = 12288000;
        break;
    case 48000:
        dacdiv = WM8960_DAC_DIV_1;
        sysclk = 12288000;
        break;
    default:
        printk("-%s(): SND RATE ERROR (%d)\n", __FUNCTION__,rate );
        return -EINVAL;
    }
    printk("-%s(): rate is %d\n", __FUNCTION__,rate );
    ret = snd_soc_dai_set_clkdiv( codec_dai,  WM8960_DACDIV, dacdiv );
    if( ret < 0 ){
        printk( "-%s(): Codec SYSCLKDIV setting error, %d\n", __FUNCTION__, ret );
        return ret;
    }
    ret = snd_soc_dai_set_fmt(codec_dai, AUDIO_FORMAT);
    if( ret < 0 ){
        printk( "-%s(): Codec DAI configuration error, %d\n", __FUNCTION__, ret );
        return ret;
    }

    //Needs to be set to proper MCLK Frequency to work correctly (Maybe make this device tree entry).
    //  Changed to 8 MHz for Triumph and Bone Island
    //ret = snd_soc_dai_set_pll(codec_dai, 0, 0, 12000000, sysclk);
    ret = snd_soc_dai_set_pll(codec_dai, 0, 0, 8000000, sysclk);
    printk("-%s()\n", __FUNCTION__ );

    //NOT SURE HOW TO DO THESE WITH MAPPINGS, so DO IT HERE
    //TURN ON LEFT ADC
    snd_soc_update_bits(codec, WM8960_POWER1, 0x008, 0x008);

    //Left input PGA Enable
    snd_soc_update_bits(codec, WM8960_POWER3, 0x020, 0x020);

    //TURN ON LEFT Analogue Input
    snd_soc_update_bits(codec, WM8960_POWER1, 0x020, 0x020);

    return 0;

}

static struct snd_soc_ops z7_bone_island_wm8960_ops = {
	.hw_params = z7_bone_island_wm8960_hw_params,
};

static int z7_bone_island_wm8960_init(struct snd_soc_pcm_runtime *rtd)
{
    struct snd_soc_codec *codec = rtd->codec;
    struct snd_soc_dapm_context *dapm = &rtd->card->dapm;
/*        
    snd_soc_dapm_nc_pin(dapm, "RINPUT1");
    snd_soc_dapm_nc_pin(dapm, "LINPUT2");
    snd_soc_dapm_nc_pin(dapm, "RINPUT2");
    snd_soc_dapm_nc_pin(dapm, "OUT3");
            
    snd_soc_dapm_new_controls( dapm, z7_bone_island_wm8960_dapm_capture_widgets, ARRAY_SIZE( z7_bone_island_wm8960_dapm_capture_widgets ) );
    snd_soc_dapm_new_controls( dapm, z7_bone_island_wm8960_dapm_playback_widgets, ARRAY_SIZE( z7_bone_island_wm8960_dapm_playback_widgets ) );
        
    snd_soc_dapm_add_routes( dapm, z7_bone_island_wm8960_audio_map, ARRAY_SIZE( z7_bone_island_wm8960_audio_map ) );
        
    snd_soc_dapm_enable_pin(dapm, "Headphone Jack");
    snd_soc_dapm_enable_pin(dapm, "Mic Jack");
    snd_soc_dapm_enable_pin(dapm, "Speaker_L");
    snd_soc_dapm_enable_pin(dapm, "Speaker_R");
            
    snd_soc_dapm_disable_pin(dapm, "Line Input 3 (FM)");
       
    snd_soc_dapm_sync( dapm );
*/
  	
/*    snd_soc_update_bits(codec, WM8960_RESET, 0x1FF, 0x000);
    snd_soc_update_bits(codec, WM8960_POWER1, 0x1FF, 0x0C0);
    snd_soc_update_bits(codec, WM8960_POWER2, 0x1FF, 0x1EF);
    snd_soc_update_bits(codec, WM8960_POWER3, 0x1FF, 0x00C);
    snd_soc_update_bits(codec, WM8960_LOUTMIX, 0x1FF, 0x100);
    snd_soc_update_bits(codec, WM8960_ROUTMIX, 0x1FF, 0x100);
    snd_soc_update_bits(codec, WM8960_LOUT1, 0x1FF, 0x179);
    snd_soc_update_bits(codec, WM8960_LOUT2, 0x1FF, 0x179);
    snd_soc_update_bits(codec, WM8960_DACCTL1, 0x1FF, 0x000);
    snd_soc_update_bits(codec, WM8960_IFACE1, 0x1FF, 0x002);
*/

    //printk (KERN_ERR "~-~- IVEIA SETUP START -~-~\n");
 
    //snd_soc_update_bits(codec, WM8960_LINVOL, 0x1FF, 0x13F);
    //snd_soc_update_bits(codec, WM8960_RINVOL, 0x1FF, 0x13F);

    //Replace with : 
    //amixer -c 0 cset name='Capture Volume' 39,39        -> Sets input Volume (0 to 63)
    //amixer -c 0 cset name='Capture Switch' off,off 

    snd_soc_update_bits(codec, WM8960_RINPATH, 0x1FF, 0x138);
    //Add in LINPATH?

    //Set ADCLRC = DACLRC, ADCLRC pin will be GPIO
    snd_soc_update_bits(codec, WM8960_IFACE2, 0x1FF, 0x40);

    snd_soc_update_bits(codec, WM8960_POWER1, 0x1FF, 0x0FC);
    snd_soc_update_bits(codec, WM8960_POWER3, 0x1FF, 0x03C);


//i2c wlf_mw 1a 19 0e8;
//i2c wlf_mw 1a 2f 02c;

    snd_soc_update_bits(codec, WM8960_POWER1, 0x1FF, 0x0e8);
    snd_soc_update_bits(codec, WM8960_POWER3, 0x1FF, 0x02c);

	snd_soc_dapm_force_enable_pin(dapm, "MICB");


    //printk (KERN_ERR "~-~- IVEIA SETUP END -~-~\n");

    return 0;
}

static struct snd_soc_dai_link z7_bone_island_wm8960_dai_link = {
	.name = "wm8960",
	.stream_name = "wm8960",
	.codec_dai_name = "wm8960-hifi",
	.dai_fmt = SND_SOC_DAIFMT_I2S |
			SND_SOC_DAIFMT_NB_NF |
			SND_SOC_DAIFMT_CBS_CFS,
	.ops = &z7_bone_island_wm8960_ops,
	.init = z7_bone_island_wm8960_init,
};

static struct snd_soc_card z7_bone_island_wm8960_card = {
	.name = "iVeia Card",
	.owner = THIS_MODULE,
	.dai_link = &z7_bone_island_wm8960_dai_link,
	.num_links = 1,
	.dapm_widgets = z7_bone_island_wm8960_widgets,
       .num_dapm_widgets = ARRAY_SIZE(z7_bone_island_wm8960_widgets),
       .dapm_routes = z7_bone_island_wm8960_routes,
       .num_dapm_routes = ARRAY_SIZE(z7_bone_island_wm8960_routes),
       .fully_routed = true,
};

static int z7_bone_island_wm8960_probe(struct platform_device *pdev)
{
	int ret;
	struct snd_soc_card *card = &z7_bone_island_wm8960_card;
	struct device_node *of_node = pdev->dev.of_node;

	if (!of_node)
		return -ENXIO;

	card->dev = &pdev->dev;

	z7_bone_island_wm8960_dai_link.codec_of_node = of_parse_phandle(of_node, "audio-codec", 0);
	z7_bone_island_wm8960_dai_link.cpu_of_node = of_parse_phandle(of_node, "cpu-dai", 0);
	z7_bone_island_wm8960_dai_link.platform_of_node = z7_bone_island_wm8960_dai_link.cpu_of_node;

	if (!z7_bone_island_wm8960_dai_link.codec_of_node ||
		!z7_bone_island_wm8960_dai_link.cpu_of_node)
		return -ENXIO;

	ret = snd_soc_register_card(card);
	if (ret)
		return ret;

	dev_info(&pdev->dev, "Atlas Zynq 7000 Sound Card probed\n");

	return 0;
}

static int z7_bone_island_wm8960_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);

	snd_soc_unregister_card(card);

	return 0;
}

static const struct of_device_id z7_bone_island_wm8960_of_match[] = {
	{ .compatible = "iveia,z7-bone-island-sound", },
	{},
};
MODULE_DEVICE_TABLE(of, z7_bone_island_wm8960_of_match);

static struct platform_driver z7_bone_island_wm8960_card_driver = {
	.driver = {
		.name = "z7-bone-island-wm8960-snd",
		.of_match_table = z7_bone_island_wm8960_of_match,
		.pm = &snd_soc_pm_ops,
	},
	.probe = z7_bone_island_wm8960_probe,
	.remove = z7_bone_island_wm8960_remove,
};
module_platform_driver(z7_bone_island_wm8960_card_driver);

MODULE_DESCRIPTION("ASoC iVeia board WM8960  driver");
MODULE_AUTHOR("Patrick Cleary");
MODULE_LICENSE("GPL");

#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/soc/qcom/smem.h>
#include <linux/seq_file.h>
#include <linux/io.h>
#include <linux/of.h>

#ifdef CONFIG_MTK_SECURITY_SW_SUPPORT
#include <sec_boot_lib.h>
#endif

#include <soc/oppo/oppo_project_newcdt.h>
#include <linux/syscalls.h>
#include <soc/oppo/oppo_project.h>
#define SMEM_PROJECT	135

#define UINT2Ptr(n)		(uint32_t *)(n)
#define Ptr2UINT32(p)		(uint32_t)(p)

#define PROJECT_VERSION			(0x1)
#define PCB_VERSION				(0x2)
#define RF_INFO					(0x3)
#define MODEM_TYPE				(0x4)
#define OPPO_BOOTMODE			(0x5)
#define SECURE_TYPE				(0x6)
#define SECURE_STAGE				(0x7)
#define OCP_NUMBER				(0x8)
#define SERIAL_NUMBER			(0x9)
#define ENG_VERSION				(0xA)
#define CONFIDENTIAL_STATUS		(0xB)
#define DTSI_NAME				(0xC)
#define AUDIO_NAME				(0xD)
#define PROJECT_TEST				(0x1F)

#ifdef VENDOR_EDIT
/*Bin.Li@BSP.Bootloader.Bootflows, 2019/05/09, Add for diff manifest*/
static const char* nfc_feature = "nfc_feature";
static const char* feature_src = "/vendor/etc/nfc/com.oppo.nfc_feature.xml";
static struct proc_dir_entry *oppo_info = NULL;
#endif
static ProjectInfoCDTType *g_project = NULL;
static EngInfoType *g_enginfo = NULL;

static ProjectInfoCDTType local_ProjectInfoCDT = {
	.nVersion					= 0,
	.nProject					= 0,
	.nDtsi					= 0,
	.nAudio					= 0,
	.nRF	        				= 0,
	.nFeature 					= {},
	.nOppoBootMode			= 0,
	.nPCB            				= 0,
	.nPmicOcp         			= {},
};

static EngInfoType local_enginfo = {
	.eng_version				= 0,
	.is_confidential				= 0,
};

static int init_project_version_newcdt(void)
{
	struct device_node *np = NULL;
	int ret = 0, i = 0;
	char feature_node[64] = {0};

	if (g_project) {
		//printk("init_project_version_newcdt has inited, nProject:%d\n", g_project->nProject);
		return ret;
	}
    	printk("init_project_version_newcdt start\n");
	
	np = of_find_node_by_name(NULL, "oppo_project");
	if(!np){
		printk("init_project_version error1");
		return -1;
	}

	ret = of_property_read_u32(np,"newcdt", &(local_ProjectInfoCDT.newcdt));
	if(ret)
	{
		printk("init_project_version_newcdt newcdt fail");
		return -1;
	}

	ret = of_property_read_u32(np,"nVersion", &(local_ProjectInfoCDT.nVersion));
	if(ret)
	{
		printk("init_project_version_newcdt nVersion fail");
		return -1;
	}
	
	ret = of_property_read_u32(np,"nProject", &(local_ProjectInfoCDT.nProject));
	if(ret)
	{
		printk("init_project_version_newcdt nProject fail");
		return -1;
	}

	ret = of_property_read_u32(np,"nDtsi", &(local_ProjectInfoCDT.nDtsi));
	if(ret)
	{
		printk("init_project_version_newcdt nDtsi fail");
		return -1;
	}

	ret = of_property_read_u32(np,"nAudio", &(local_ProjectInfoCDT.nAudio));
	if(ret)
	{
		printk("init_project_version_newcdt nAudio fail");
		return -1;
	}

	ret = of_property_read_u32(np,"nRF", &(local_ProjectInfoCDT.nRF));
	if(ret)
	{
		printk("init_project_version_newcdt nRF fail");
		//return -1;
	}

	for (i = 0; i < FEATURE_COUNT; i++) {
		sprintf(feature_node, "nFeature%d", i);
		ret = of_property_read_u32(np, feature_node, &(local_ProjectInfoCDT.nFeature[i]));
		if(ret)
		{
			printk("init_project_version_newcdt nFeature[%d]: %s %d fail",
				i, feature_node, local_ProjectInfoCDT.nFeature[i]);
			//return -1;
		}
	}

	ret = of_property_read_u32(np,"nPCB", &(local_ProjectInfoCDT.nPCB));
	if(ret)
	{
		printk("init_project_version_newcdt nPCB fail");
		return -1;
	}
	
	//for eng version
	ret = of_property_read_u32(np,"eng_version", &(local_enginfo.eng_version));
	if(ret)
	{
		printk("init_project_version_newcdt eng_version fail");
		return -1;
	}

	ret = of_property_read_u32(np,"is_confidential", &(local_enginfo.is_confidential));
	if(ret)
	{
		printk("init_project_version_newcdt is_confidential fail");
		return -1;
	}

	g_project = &local_ProjectInfoCDT;
	g_enginfo = &local_enginfo;
	
	pr_err("KE new cdt newcdt:%d nVersion:%d Project:%d, Dtsi:%d, Audio:%d, nRF:%d, PCB:%d Feature0-9:%d %d %d %d %d %d %d %d %d %d eng_version:%d confidential:%d\n",
		g_project->nVersion,
		g_project->newcdt,
		g_project->nProject,
		g_project->nDtsi,
		g_project->nAudio,
		g_project->nRF,
		g_project->nPCB,
		g_project->nFeature[0],
		g_project->nFeature[1],
		g_project->nFeature[2],
		g_project->nFeature[3],
		g_project->nFeature[4],
		g_project->nFeature[5],
		g_project->nFeature[6],
		g_project->nFeature[7],
		g_project->nFeature[8],
		g_project->nFeature[9],
		g_enginfo->eng_version,
		g_enginfo->is_confidential);
	
	pr_err("OCP: %d 0x%x %c %d 0x%x %c\n",
		g_project->nPmicOcp[0],
		g_project->nPmicOcp[1],
		g_project->nPmicOcp[2],
		g_project->nPmicOcp[3],
		g_project->nPmicOcp[4],
		g_project->nPmicOcp[5]);
	
	return ret;
	
}

unsigned int get_cdt_version_newcdt(void)
{
	return g_project ? g_project->nVersion : 0;
}

unsigned int get_PCB_Version_newcdt(void)
{
	return g_project ? g_project->nPCB : PCB_UNKNOWN;
}

unsigned int get_project_newcdt(void)
{
	return g_project?g_project->nProject:0;
}

unsigned int is_project_newcdt(int project)
{
	return (get_project_newcdt() == project?1:0);
}

unsigned char get_Oppo_Boot_Mode_newcdt(void)
{
	return g_project?g_project->nOppoBootMode:0;
}

unsigned int get_rf_info_newcdt(void)
{
	return g_project?g_project->nRF:0;
}

int rpmb_is_enable_newcdt(void)
{
	return 0;
}

#if 0
static void init_eng_version_newcdt(void)
{

}
#endif

int get_eng_version_newcdt(void)
{
	return g_enginfo ? g_enginfo->eng_version : RELEASE;
}

int is_confidential_newcdt(void)
{
	return g_enginfo ? g_enginfo->is_confidential : true;
}

unsigned int get_oppo_feature_newcdt(unsigned int feature_num)
{
	if (feature_num > (FEATURE_COUNT - 1))
		return 0;
	
	return g_project ? g_project->nFeature[feature_num] : 0;
}

unsigned int get_dtsiNo_newcdt(void)
{
	return g_project ? g_project->nDtsi: 0;
}

unsigned int get_audio_project_newcdt(void)
{
	return g_project ? g_project->nAudio : 0;
}

bool oppo_daily_build_newcdt(void)
{
#if 0

#if defined(CONFIG_OPPO_BUILD_USER)
    return false;
#else
    int eng_version = 0;

    eng_version = get_eng_version();
    if ((ALL_NET_CMCC_TEST == eng_version) || (ALL_NET_CMCC_FIELD == eng_version) ||
        (ALL_NET_CU_TEST == eng_version) || (ALL_NET_CU_FIELD == eng_version) ||
        (ALL_NET_CT_TEST == eng_version) || (ALL_NET_CT_FIELD == eng_version))
        return false;
	else
    	return true;
#endif /* CONFIG_OPPO_BUILD_USER */

#else
	return false;
#endif
}

static void dump_ocp_info(struct seq_file *s)
{
	oppo_project_init_newcdt();

	if (!g_project)
		return;

	seq_printf(s, "%d 0x%x %d 0x%x %c %c",
		g_project->nPmicOcp[0],
		g_project->nPmicOcp[1],
		g_project->nPmicOcp[2],
		g_project->nPmicOcp[3],
		g_project->nPmicOcp[4],
		g_project->nPmicOcp[5]);
}

/*Kun.Wang@BSP.Fingerprint.Secure 2019/11/27 Add serialID for fastboot unlock*/
#define SERIALNO_LEN 16
extern char *saved_command_line;
static void dump_serial_info(struct seq_file *s)
{
        char * ptr;
        char serialno[SERIALNO_LEN+1] = {0};

        ptr = strstr(saved_command_line, "androidboot.serialno=");
        ptr += strlen("androidboot.serialno=");
        strncpy(serialno, ptr, SERIALNO_LEN);
        serialno[SERIALNO_LEN] = '\0';
        seq_printf(s, "0x%s", serialno);

}

static void dump_eng_version(struct seq_file *s)
{
	seq_printf(s, "%d", get_eng_version_newcdt());
	return;
}

static void dump_confidential_status(struct seq_file *s)
{
	seq_printf(s, "%d", is_confidential_newcdt());
	return;
}

static void dump_dtsiname(struct seq_file *s)
{
	seq_printf(s, "%d", get_dtsiNo_newcdt());
	return;
}

static void dump_audioname(struct seq_file *s)
{
	seq_printf(s, "%d", get_audio_project_newcdt());
	return;
}

static void dump_secure_type(struct seq_file *s)
{
#ifndef VENDOR_EDIT
//wangkun@Prd6.BaseDrv, 2019/12/09,, Add for MTK secureType node
    uint32_t secure_oem_config = 0;
    printk(KERN_EMERG "lycan test secure_oem_config 0x%x\n", secure_oem_config);
#else
#ifdef CONFIG_MTK_SECURITY_SW_SUPPORT
    int secure_oem_config = 0;
    secure_oem_config = sec_schip_enabled();
    printk(KERN_EMERG "lycan test secure_oem_config %d\n", secure_oem_config);
#else
    uint32_t secure_oem_config = 0;
#endif
#endif /* VENDOR_EDIT */
    seq_printf(s, "%d", secure_oem_config);
}

static void dump_secure_stage(struct seq_file *s)
{
#if 0
#define OEM_SEC_ENABLE_ANTIROLLBACK_REG 0x78019c

	void __iomem *oem_config_base = NULL;
	uint32_t secure_oem_config = 0;

	oem_config_base = ioremap(OEM_SEC_ENABLE_ANTIROLLBACK_REG, 4);
	if (oem_config_base) {
		secure_oem_config = __raw_readl(oem_config_base);
		iounmap(oem_config_base);
	}

	seq_printf(s, "%d", secure_oem_config);
#endif
}

static void update_manifest(struct proc_dir_entry *parent)
{
	static const char* manifest_src[2] = {
		"/vendor/etc/vintf/manifest_ssss.xml",
		"/vendor/etc/vintf/manifest_dsds.xml",
	};
	mm_segment_t fs;
	char * substr = strstr(saved_command_line, "simcardnum.doublesim=");

	if(!substr)
		return;

	substr += strlen("simcardnum.doublesim=");

	fs = get_fs();
	set_fs(KERNEL_DS);

	if (parent) {
		if (substr[0] == '0')
			proc_symlink("manifest", parent, manifest_src[0]);//single sim
		else
			proc_symlink("manifest", parent, manifest_src[1]);
	}

	set_fs(fs);
}

#ifdef VENDOR_EDIT
/*Bin.Li@BSP.Bootloader.Bootflows, 2019/05/09, Add for diff manifest*/
static int __init update_feature(void)
{
	mm_segment_t fs;
	fs = get_fs();
	pr_err("%s: newcdt --  Operator Version [%d]\n", __func__, (get_project()));
	set_fs(KERNEL_DS);
	if (oppo_info) {
		if (get_oppo_feature(1) & 0x01) {
			proc_symlink(nfc_feature, oppo_info, feature_src);
		}
	}
	set_fs(fs);
	return 0;
}
late_initcall(update_feature);
#endif

static int project_read_func(struct seq_file *s, void *v)
{
	void *p = s->private;

	switch(Ptr2UINT32(p)) {
	case PROJECT_VERSION:
		seq_printf(s, "%d", get_project_newcdt());
		break;
	case PCB_VERSION:
		seq_printf(s, "%d", get_PCB_Version_newcdt());
		break;
	case OPPO_BOOTMODE:
		seq_printf(s, "%d", get_Oppo_Boot_Mode_newcdt());
		break;
	case RF_INFO:
		seq_printf(s, "%d", get_rf_info_newcdt());
		break;
	case SECURE_TYPE:
		dump_secure_type(s);
		break;
	case SECURE_STAGE:
		dump_secure_stage(s);
		break;
	case OCP_NUMBER:
		dump_ocp_info(s);
		break;
	case SERIAL_NUMBER:
		dump_serial_info(s);
		break;
	case ENG_VERSION:
		dump_eng_version(s);
		break;
	case CONFIDENTIAL_STATUS:
		dump_confidential_status(s);
		break;
	case DTSI_NAME:
		dump_dtsiname(s);
		break;
	case AUDIO_NAME:
		dump_audioname(s);
		break;
	default:
		seq_printf(s, "not support\n");
		break;
	}

	return 0;
}

static int projects_open(struct inode *inode, struct file *file)
{
	return single_open(file, project_read_func, PDE_DATA(inode));
}

static const struct file_operations project_info_fops = {
	.owner = THIS_MODULE,
	.open  = projects_open,
	.read  = seq_read,
	.release = single_release,
};

int oppo_project_init_newcdt(void)
{
	struct proc_dir_entry *p_entry;
	int ret = 0;

	if (oppo_info) {
		//pr_info("%s has inited\n", __func__);
		return ret;
	}
	
	ret = init_project_version_newcdt();
	if (ret) {
		pr_err("%s init_project fail, ret:%d\n", __func__, ret);
		goto init_project_fail;
	}
	
	oppo_info = proc_mkdir("oppoVersion", NULL);
	if (!oppo_info) {
		goto error_init;
	}

	p_entry = proc_create_data("prjName", S_IRUGO, oppo_info, &project_info_fops, UINT2Ptr(PROJECT_VERSION));
	if (!p_entry)
		goto error_init;

	p_entry = proc_create_data("pcbVersion", S_IRUGO, oppo_info, &project_info_fops, UINT2Ptr(PCB_VERSION));
	if (!p_entry)
		goto error_init;

	p_entry = proc_create_data("oppoBootmode", S_IRUGO, oppo_info, &project_info_fops, UINT2Ptr(OPPO_BOOTMODE));
	if (!p_entry)
		goto error_init;

	p_entry = proc_create_data("RFType", S_IRUGO, oppo_info, &project_info_fops, UINT2Ptr(RF_INFO));
	if (!p_entry)
		goto error_init;

	p_entry = proc_create_data("secureType", S_IRUGO, oppo_info, &project_info_fops, UINT2Ptr(SECURE_TYPE));
	if (!p_entry)
		goto error_init;

	p_entry = proc_create_data("secureStage", S_IRUGO, oppo_info, &project_info_fops, UINT2Ptr(SECURE_STAGE));
	if (!p_entry)
		goto error_init;

	p_entry = proc_create_data("ocp", S_IRUGO, oppo_info, &project_info_fops, UINT2Ptr(OCP_NUMBER));
	if (!p_entry)
		goto error_init;

	p_entry = proc_create_data("serialID", S_IRUGO, oppo_info, &project_info_fops, UINT2Ptr(SERIAL_NUMBER));
	if (!p_entry)
		goto error_init;

	p_entry = proc_create_data("engVersion", S_IRUGO, oppo_info, &project_info_fops, UINT2Ptr(ENG_VERSION));
	if (!p_entry)
		goto error_init;

	p_entry = proc_create_data("isConfidential", S_IRUGO, oppo_info, &project_info_fops, UINT2Ptr(CONFIDENTIAL_STATUS));
	if (!p_entry)
		goto error_init;

	p_entry = proc_create_data("dtsiName", S_IRUGO, oppo_info, &project_info_fops, UINT2Ptr(DTSI_NAME));
	if (!p_entry)
		goto error_init;

	p_entry = proc_create_data("audioName", S_IRUGO, oppo_info, &project_info_fops, UINT2Ptr(AUDIO_NAME));
	if (!p_entry)
		goto error_init;

	p_entry = proc_create_data("test", S_IRUGO, oppo_info, &project_info_fops, UINT2Ptr(PROJECT_TEST));
	if (!p_entry)
		goto error_init;

	/*update single or double cards*/
	update_manifest(oppo_info);
	pr_err("%s success\n", __func__);
	return 0;

error_init:
	remove_proc_entry("oppoVersion", NULL);
init_project_fail:
	oppo_info = NULL;
	g_project = NULL;
	return -ENOENT;
}

//arch_initcall(oppo_project_init);

MODULE_DESCRIPTION("OPPO project newcdt version");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Joshua <gyx@oppo.com>");

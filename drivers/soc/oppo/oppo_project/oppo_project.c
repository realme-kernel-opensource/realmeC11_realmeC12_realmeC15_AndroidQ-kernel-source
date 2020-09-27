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
#include <soc/oppo/oppo_project.h>

unsigned int get_cdt_version(void)
{
	if (is_new_cdt()) {
		return get_cdt_version_newcdt();
	} else {
		return 0;
	}
}

unsigned int get_project(void)
{
	if (is_new_cdt()) {
		return get_project_newcdt();
	} else {
		return get_project_oldcdt();
	}
}
EXPORT_SYMBOL(get_project);

unsigned int is_project(int project)
{
	if (is_new_cdt()) {
		return is_project_newcdt(project);
	} else {
		return is_project_oldcdt(project);
	}
}

unsigned int get_audio_project(void)
{
	if (is_new_cdt()) {
		return get_audio_project_newcdt();
	} else {
		return 0;
	}
}

unsigned int get_PCB_Version(void)
{
	if (is_new_cdt()) {
		return get_PCB_Version_newcdt();
	} else {
		return get_PCB_Version_oldcdt();
	}
}
EXPORT_SYMBOL(get_PCB_Version);

unsigned int get_Modem_Version(void)
{
	if (is_new_cdt()) {
		return get_rf_info_newcdt();
	} else {
		return get_Modem_Version_oldcdt();
	}
}

unsigned int get_Operator_Version(void)
{
	if (is_new_cdt()) {
		return 0;
	} else {
		return get_Operator_Version_oldcdt();
	}
}

unsigned int get_oppo_feature(unsigned int feature_num)
{
	if (is_new_cdt()) {
		return get_oppo_feature_newcdt(feature_num);
	} else {
		return 0;
	}
}

unsigned int get_eng_version(void)
{
	if (is_new_cdt()) {
		return get_eng_version_newcdt();
	} else {
		return get_eng_version_oldcdt();
	}
}

int is_confidential(void)
{
	if (is_new_cdt()) {
		return is_confidential_newcdt();
	} else {
		return is_confidential_oldcdt();
	}
}

bool oppo_daily_build(void)
{
	if (is_new_cdt()) {
		return oppo_daily_build_newcdt();
	} else {
		return oppo_daily_build_oldcdt();
	}
}

unsigned int is_new_cdt(void)
{	
	struct device_node *np = NULL;
	int ret = 0;
	unsigned int newcdt = 0;
	static int cdt_version = CDT_VERSION_NONE;

	if (cdt_version == CDT_VERSION_NEW) {
		return 1;
	} else if (cdt_version == CDT_VERSION_OLD) {
		return 0;
	}
	
    	pr_err("%s start\n", __func__);
	
	np = of_find_node_by_name(NULL, "oppo_project");
	if (!np) {
		pr_err("is_new_cdt get oppo_project node fail");
		cdt_version = CDT_VERSION_NONE;
		return 0;
	}

	ret = of_property_read_u32(np,"newcdt", &newcdt);
	if (ret) {
		pr_err("%s no newcdt node\n", __func__);
		cdt_version = CDT_VERSION_OLD;
	} else if (newcdt) {
		cdt_version = CDT_VERSION_NEW;
	} else {
		cdt_version = CDT_VERSION_OLD;
	}

	if (cdt_version == CDT_VERSION_NEW) {
		ret = oppo_project_init_newcdt();
		if (ret) {
			pr_err("%s CRITICAL_ERROR, init new cdt fail\n", __func__);
			cdt_version = CDT_VERSION_NONE;
		} else {
			pr_err("%s init new cdt success\n", __func__);
		}
		return 1;
	} else {
		ret = oppo_project_init_oldcdt();
		if (ret) {
			pr_err("%s  CRITICAL_ERROR, init old cdt fail\n", __func__);
			cdt_version = CDT_VERSION_NONE;
		} else {
			pr_err("%s init old cdt success\n", __func__);
		}
		return 0;
	}
}

static int __init oppo_project_init(void)
{
	is_new_cdt();
	return 0;
}

core_initcall(oppo_project_init);

MODULE_DESCRIPTION("OPPO project version");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Joshua <gyx@oppo.com>");

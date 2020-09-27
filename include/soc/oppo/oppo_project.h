#ifndef _OPPO_PROJECT_H_
#define _OPPO_PROJECT_H_

#include <soc/oppo/oppo_project_oldcdt.h>
#include <soc/oppo/oppo_project_newcdt.h>

enum CDT_VERSION {
	CDT_VERSION_NONE,
	CDT_VERSION_OLD,
	CDT_VERSION_NEW,
};
unsigned int get_cdt_version(void);
unsigned int get_project(void);
unsigned int is_project(int project);
unsigned int get_audio_project(void);
unsigned int get_PCB_Version(void);
unsigned int get_Modem_Version(void);
unsigned int get_Operator_Version(void);
unsigned int get_oppo_feature(unsigned int feature_num);
unsigned int get_eng_version(void);
int is_confidential(void);
unsigned int is_new_cdt(void);

#endif

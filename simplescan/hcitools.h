// HCI tools protoypes

#ifdef __cplusplus
extern "C" {
#endif

void hcitool_rstat(int ctl, int hdev, char *opt);
void hcitool_scan(int ctl, int hdev, char *opt);
void hcitool_le_addr(int ctl, int hdev, char *opt);
void hcitool_le_adv(int ctl, int hdev, char *opt);
void hcitool_no_le_adv(int ctl, int hdev, char *opt);
void hcitool_le_states(int ctl, int hdev, char *opt);
void hcitool_iac(int ctl, int hdev, char *opt);
void hcitool_auth(int ctl, int hdev, char *opt);
void hcitool_encrypt(int ctl, int hdev, char *opt);
void hcitool_up(int ctl, int hdev, char *opt);
void hcitool_down(int ctl, int hdev, char *opt);
void hcitool_reset(int ctl, int hdev, char *opt);
void hcitool_ptype(int ctl, int hdev, char *opt);
void hcitool_lp(int ctl, int hdev, char *opt);
void hcitool_lm(int ctl, int hdev, char *opt);
void hcitool_aclmtu(int ctl, int hdev, char *opt);
void hcitool_scomtu(int ctl, int hdev, char *opt);
void hcitool_features(int ctl, int hdev, char *opt);
void hcitool_name(int ctl, int hdev, char *opt);
void hcitool_class(int ctl, int hdev, char *opt);
void hcitool_voice(int ctl, int hdev, char *opt);
void hcitool_delkey(int ctl, int hdev, char *opt);
void hcitool_oob_data(int ctl, int hdev, char *opt);
void hcitool_commands(int ctl, int hdev, char *opt);
void hcitool_version(int ctl, int hdev, char *opt);
void hcitool_inq_tpl(int ctl, int hdev, char *opt);
void hcitool_inq_mode(int ctl, int hdev, char *opt);
void hcitool_inq_data(int ctl, int hdev, char *opt);
void hcitool_inq_type(int ctl, int hdev, char *opt);
void hcitool_inq_parms(int ctl, int hdev, char *opt);
void hcitool_page_parms(int ctl, int hdev, char *opt);
void hcitool_page_to(int ctl, int hdev, char *opt);
void hcitool_afh_mode(int ctl, int hdev, char *opt);
void hcitool_ssp_mode(int ctl, int hdev, char *opt);
void hcitool_revision(int ctl, int hdev, char *opt);
void hcitool_block(int ctl, int hdev, char *opt);
void hcitool_unblock(int ctl, int hdev, char *opt);


#ifdef __cplusplus
}
#endif


void la_test_args (opt_args (*args), struct command_result *res);
bool logicanalyzer_setup(void);
int logicanalyzer_status(void);
uint8_t logicanalyzer_dump(uint8_t *txbuf);
bool logic_analyzer_is_done(void);
bool logic_analyzer_arm(uint32_t samples, int trigger_pin);

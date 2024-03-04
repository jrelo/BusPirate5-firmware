typedef struct arg_var_struct {
    bool has_arg;
    bool has_value;
    uint32_t value_pos;
    bool error;
    uint8_t number_format;
} arg_var_t;

bool ui_args_find_flag_novalue(char flag, arg_var_t *arg);
bool ui_args_find_flag_string(char flag, arg_var_t *arg, uint32_t max_len, char *value);
bool ui_args_find_flag_uint32(char flag, arg_var_t *arg, uint32_t *value);
bool ui_args_find_string(arg_var_t *arg, uint32_t max_len, char *value);
bool ui_args_find_uint32(arg_var_t *arg, uint32_t *value);
bool ui_args_find_string_discard(arg_var_t *arg, bool discard, uint32_t max_len, char *value);
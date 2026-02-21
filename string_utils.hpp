#pragma once

int my_strlen(const char *s);
void my_strcpy(const char *src, char *dst, int dst_size);
bool my_streq(const char *s1, const char *s2);
void my_strcat(const char *src, char *dst, int dst_size);
void my_strcat_front(const char *src, char *dst, int dst_size);

//追加
int my_char2int(const char c);
int my_str2int(const char *src);
char my_int2char(int a1);
void my_int2str(int a1, char *dst, int dst_size);
bool my_isnum(char c);
bool my_issymbol(char c);

bool is_space(char c);
bool is_digit(char c);
bool is_alpha(char c);
bool is_ident_head(char c);
bool is_ident_tail(char c);
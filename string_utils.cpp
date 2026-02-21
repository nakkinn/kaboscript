#include <iostream>
#include "string_utils.hpp"

int my_strlen(const char *s){
    int i = 0;
    while(s[i]!='\0'){
        i++;
    }
    return i;
}

void my_strcpy(const char *src, char *dst, int dst_size){
    int i = 0;
    while(i<dst_size-1 && src[i]!='\0'){
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

bool my_streq(const char *s1, const char *s2){
    if(my_strlen(s1) != my_strlen(s2)){
        return false;
    }
    for(int i=0; i<my_strlen(s1); i++){
        if(s1[i] != s2[i])  return false;
    }
    return true;
}

void my_strcat(const char *src, char *dst, int dst_size){
    int i = 0, j = my_strlen(dst);
    while(j<dst_size-1 && src[i]!='\0'){
        dst[j] = src[i];
        i++;
        j++;
    }
    dst[j] = '\0';
}

void my_strcat_front(const char *src, char *dst, int dst_size){
    int d = std::min(my_strlen(src),dst_size-my_strlen(dst));
    for(int i=my_strlen(dst); i>=0; i--){
        dst[i+d] = dst[i];
    }
    for(int i=0; i<d; i++){
        dst[i] = src[i];
    }
}

//0～9の文字列をint型に変換
int my_char2int(const char c){
    if(c=='0')  return 0;
    if(c=='1')  return 1;
    if(c=='2')  return 2;
    if(c=='3')  return 3;
    if(c=='4')  return 4;
    if(c=='5')  return 5;
    if(c=='6')  return 6;
    if(c=='7')  return 7;
    if(c=='8')  return 8;
    if(c=='9')  return 9;
    return -1;
}

//自然数の文字列をint型に変換
int my_str2int(const char *src){
    int result = 0;
    for(int i=0; i<my_strlen(src); i++){
        result += my_char2int(src[i]);
        if(i!=my_strlen(src)-1)   result *= 10;
    }
    return result;
}

//1桁の数を文字に変換
char my_int2char(int a1){
    if(a1==0)   return '0';
    if(a1==1)   return '1';
    if(a1==2)   return '2';
    if(a1==3)   return '3';
    if(a1==4)   return '4';
    if(a1==5)   return '5';
    if(a1==6)   return '6';
    if(a1==7)   return '7';
    if(a1==8)   return '8';
    if(a1==9)   return '9';
    return '\0';
}

//数値を文字列に変換
void my_int2str(int a1, char *dst, int dst_size){
    dst[0] = '\0';

    bool neg = false;

    if(a1==0){
        dst[0] = '0';
        dst[1] = '\0';
    }else if(a1<0){
        neg = true;
        a1 *= -1;
    }

    while(a1>0){
        char tmp[2];
        tmp[0] = my_int2char(a1%10);
        tmp[1] = '\0';
        my_strcat_front(tmp, dst, dst_size);
        a1 /= 10;
    }

    if(neg){
        char tmp[2];
        tmp[0] = '-';
        tmp[1] = '\0';
        my_strcat_front(tmp, dst, dst_size);
    }
}

//cが0~9ならばtrue
bool my_isnum(char c){
    if(c=='0')  return true;
    if(c=='1')  return true;
    if(c=='2')  return true;
    if(c=='3')  return true;
    if(c=='4')  return true;
    if(c=='5')  return true;
    if(c=='6')  return true;
    if(c=='7')  return true;
    if(c=='8')  return true;
    if(c=='9')  return true;
    return false;
}

bool my_issymbol(char c){
    if(c=='+')  return true;
    if(c=='-')  return true;
    if(c=='*')  return true;
    if(c=='(')  return true;
    if(c==')')  return true;
    return false;
}

bool is_space(char c){
    return c==' ' || c=='\n' || c=='\t' || c=='\r';
}

bool is_digit(char c){
    return c>='0' && c<='9';
}

bool is_alpha(char c){
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool is_ident_head(char c){
    return is_alpha(c) || c=='_';
}

bool is_ident_tail(char c){
    return is_ident_head(c) || is_digit(c);
}
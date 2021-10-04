#include "include/shell.h"

void pstr(const char* str,uint64_t color, uint16_t x, uint16_t y,ozone::terminal_size_t t_size)
{
    for(uint64_t i=0;str[i];i++)
    {
        if(str[i]=='\n')
        {
            y++;
            x=0;
            continue;
        }
        
        ozone::user::put_char(str[i],color,x,y);
        x++;
        x%=t_size.x;
        y=x==0?y+1:y;
    }
        
}
void rstr(char* str,uint64_t max_str_len,uint16_t x, uint16_t y, ozone::terminal_size_t t_size)
{
    auto gb = complete_color(rgb(0xaa,0xff,0xaa),rgb(0,0,0));
    for(uint64_t i=0;i<max_str_len;i++)
    {
        str[i]=ozone::user::get_char();
        if(str[i]=='\n'||str[i]==0)
        {
            str[i]=0;
            break;
        }
        ozone::user::put_char(str[i],gb,x,y);
        x++;
        x%=t_size.x;
        y=x==0?y+1:y;
    }
}



constexpr uint64_t COMMAND_LENGTH = 1024;
int main()
{
    auto screen_size = ozone::user::get_stdout_handle();
    ozone::user::get_stdin_handle();
    auto wb = complete_color(rgb(0xaa,0xaa,0xaa),rgb(0,0,0));
    pstr("Welcome to OSH ",wb,0,0,screen_size);
    char cmd[COMMAND_LENGTH];
    uint64_t i =1 ;
    while(true)
    {
        rstr(cmd,COMMAND_LENGTH,0,i,screen_size);
        pstr(cmd,wb,0,i+1,screen_size);
        i+=2;
        //execute(cmd);
    }
    
    return 0;
}
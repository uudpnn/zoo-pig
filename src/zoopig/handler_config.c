#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* m_file_get_cmdval_str(char* c_cmd)
        //      must free after getting
{
        //mcos("[%lu]:use m_file_get_cmdval_str\n",pthread_self());
        FILE *f=NULL;
        char linebuf[4096+1]="";
        char* res=NULL;

        if (NULL!=c_cmd)
        {
          f=popen(c_cmd, "r");
          if (NULL!=f)
          {
            //printf("open ok.\n");
            if (fgets(linebuf, sizeof(linebuf)-1, f)!=NULL)
            {
              //printf("\n[%s]\n", linebuf);
              m_file_getstr_from_linebuf(linebuf);
              //printf("\n[%s]\n", linebuf);

              if (0<strlen(linebuf))
              {
                //printf("[%s]\n", linebuf);
                res=strdup(linebuf);
              }
            }
            pclose(f);
          }
        }
        return res;
}
void m_file_getstr_from_linebuf(char* v_linebuf)
{
        int idx=0;

        if (NULL!=v_linebuf)
        {
          while (
                (idx<strlen(v_linebuf))
                &&(
                        ('\t'==v_linebuf[idx])
                        ||(0x1F<((unsigned char)v_linebuf[idx]))
                )
          )
          {
            //printf("[0x%02X]\n", (g_t_bit8u)linebuf[idx]);
            idx++;
          }
          //printf("[%u, %u]\n", idx, strlen(linebuf));
          if (idx<strlen(v_linebuf))
          {
            v_linebuf[idx]=0;
          }
        }
}

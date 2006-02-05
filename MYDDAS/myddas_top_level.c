/*************************************************************************
*									 *
*	 YAP Prolog 							 *
*									 *
*	Yap Prolog was developed at NCCUP - Universidade do Porto	 *
*									 *
* Copyright L.Damas, V.S.Costa and Universidade do Porto 1985-1997	 *
*									 *
**************************************************************************
*									 *
* File:		myddas_top_level.c					 *
* Last rev:	27/01/06						 *
* mods:									 *
* comments:	Top Level of the MYDDAS Interface                        *
*									 *
*************************************************************************/

#if defined MYDDAS_TOP_LEVEL && defined MYDDAS_MYSQL

#include "Yap.h"
#include "Yatom.h"
#include "myddas_util.h"
#include "myddas_structs.h"
#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/times.h>

//void myddas_top_level_preprocess(char *);
STATIC_PROTO(ulong start_timer,(void));
STATIC_PROTO(int com_quit,(char *,char*));
STATIC_PROTO(int myddas_top_level_command,(char *,char *));
STATIC_PROTO(int c_db_top_level,(void));

typedef struct {
  const char *name;		/* User printable name of the function. */
  char cmd_char;		/* msql command character */
  int (*func)(char *str,char *); /* Function to call to do the job. */
  //bool takes_params;		/* Max parameters for command */
  const char *doc;		/* Documentation for this function.  */
} COMMANDS;


static COMMANDS commands[] = {
  //  { "?",      '?', com_help,   1, "Synonym for `help'." },
  { "exit",   'q', com_quit,  "Exit MYDDAS Top Level. Same as quit."},
  { "quit",   'q', com_quit,  "Quit MYDDAS Top Level." },
  // End of the vector
  { (char *)NULL,       0, 0, ""}
};



static int
c_db_top_level(void) {
  Term arg_conn = Deref(ARG1);
  Term arg_res_set_mode = Deref(ARG2);
  
  MYSQL *con = (MYSQL *) IntegerOfTerm(arg_conn);
  char *res_set_mode = AtomName(AtomOfTerm(arg_res_set_mode));
  char *line;
  int quit;
  
  Yap_regp->MYDDAS_GLOBAL_POINTER->myddas_top_level_connection = myddas_util_search_connection(con);

  printf ("\n");
  for (;;) {
    /* Ignore empty lines */
    while (strlen(line = readline("mysql> ")) == 0) {
      free(line);         
    }
    //myddas_top_level_preprocess(line);
    
    add_history(line);
    quit = myddas_top_level_command(line,res_set_mode);
    free(line);           
    if (quit == -1)
      return TRUE;
  }
  
}

static 
ulong start_timer(void){
  struct tms tms_tmp;
  return times(&tms_tmp);
}



/* void */
/* myddas_top_level_preprocess(char *line); */

static int    
myddas_top_level_command(char *line,char *res_set_mode){
  
  int i;
  MYSQL *conn;
  MYSQL_RES *res_set;

  if ( (line[0] == '#' ||
	(line[0] == '-' && line[1] == '-') ||
	line[0] == 0))
    return 0;					// Skip comment lines


  for (i=0;commands[i].name!=NULL;i++)
    if (!strcmp(commands[i].name,line))
      {
	int quit = (*(commands[i].func))(NULL,NULL);
	if (quit == -1)
	  return -1;
      }

  int length=strlen(line);

  conn = (MYSQL *) (Yap_regp->MYDDAS_GLOBAL_POINTER->myddas_top_level_connection->connection);

  ulong start = start_timer();
  if (mysql_real_query(conn, line, length) != 0){
    printf ("ERROR %d (%s): %s\n",mysql_errno(conn),mysql_sqlstate(conn),mysql_error(conn));
    
  }
  else{
    if (strcmp(res_set_mode,"store_result")==0) //True
      res_set = mysql_store_result(conn);
    else
      res_set = mysql_use_result(conn);
    ulong end = start_timer();
    myddas_util_table_write(res_set);
    printf ("%lld rows in set (%ld sec)\n",mysql_num_rows(res_set),end-start);
    mysql_free_result(res_set);
  }
  return 0;
}

int com_quit(char *nill,char *null){
  Yap_regp->MYDDAS_GLOBAL_POINTER->myddas_top_level_connection = NULL;
  printf ("Bye\n");
  return -1;
}

void Yap_InitMYDDAS_TopLevelPreds(void)
{
  Yap_InitCPred("c_db_top_level", 2, c_db_top_level,  SafePredFlag|SyncPredFlag|HiddenPredFlag);
  
}

#endif



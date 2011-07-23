/**
* Author:
* 	Pierre Lindenbaum PhD
* Date
*	July 2011
* Contact:
* 	plindenbaum@yahoo.fr
* WWW:
* 	http://plindenbaum.blogspot.com
*	http://dev.mysql.com/doc/refman/5.1/en/writing-full-text-plugins.html
* Motivation:
* 	A mysql full-text plugin finding rs## numbers
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <ctype.h>
#include <mysql/plugin.h>

#if !defined(__attribute__) && (defined(__cplusplus) || !defined(__GNUC__)  || __GNUC__ == 2 && __GNUC_MINOR__ < 8)
#define __attribute__(A)
#endif


/*** 
http://dev.mysql.com/doc/refman/5.1/en/writing-full-text-plugins.html

The general plugin descriptor in the library descriptor names the initialization and deinitialization 
that the server should invoke when it loads and unloads the plugin. For simple_parser, these functions
do nothing but return zero to indicate that they succeeded: */

static int bioparser_plugin_init(void *arg __attribute__((unused)))
	{
  	return(0);
	}

static int bioparser_plugin_deinit(void *arg __attribute__((unused)))
	{
 	return(0);
	}


/* Initialize the parser on the first use in the query */
static int bioparser_init(MYSQL_FTPARSER_PARAM *param
                              __attribute__((unused)))
	{
  	return(0);
	}


/* dispose the parser on at the end of the query */
static int bioparser_deinit(MYSQL_FTPARSER_PARAM *param
                                __attribute__((unused)))
	{
  	return(0);
	}


/* Pass a word back to the server. */
static void my_add_word(
	MYSQL_FTPARSER_PARAM *param,/* mysql context */
        char *word, /* word */
        size_t len /* word length */
	)
	{
	
 	MYSQL_FTPARSER_BOOLEAN_INFO bool_info={ FT_TOKEN_WORD, 0, 0, 0, 0, ' ', 0 };
        if (param->mode == MYSQL_FTPARSER_FULL_BOOLEAN_INFO)
		{
		bool_info.yesno = 1;
	        }
        param->mysql_add_word(param, word, len, &bool_info);
        }


#define IS_DELIM(c) (isspace((c)) || ispunct((c)))
/* Parse a document or a search query. */
static int bioparser_parse(MYSQL_FTPARSER_PARAM *param)
  {
  char *curr=param->doc;
  const char *begin=param->doc;
  const char *end= begin + param->length;

  param->flags = MYSQL_FTFLAGS_NEED_COPY;
  while(curr+2<end)
	{
	if(tolower(*curr)=='r' &&
           tolower(*(curr+1))=='s' &&
           isdigit(*(curr+2)) &&
           (curr==begin ||  IS_DELIM(*(curr-1) ) )
           )
	  {
          char* p=curr+2;
          while(p!=end && isdigit(*p))
		{
		++p;
		}
	  if(p==end || IS_DELIM(*p))
		{
		my_add_word(param,curr,p-curr);
		}
          curr=p; 
	  }
        else
	  {
          curr++;
          }
	}
  
  return 0;
  }
  


/* Plugin type-specific descriptor */
static struct st_mysql_ftparser bioparser_descriptor=
  {
  MYSQL_FTPARSER_INTERFACE_VERSION, /* interface version      */
  bioparser_parse,              /* parsing function       */
  bioparser_init,               /* parser init function   */
  bioparser_deinit              /* parser deinit function */
  };

/* Plugin status variables for SHOW STATUS (empty ) */
static struct st_mysql_show_var simple_status[]=
	{
	{0,0,0}
	};

/* Plugin system variables. (empty) */
static struct st_mysql_sys_var* simple_system_variables[]= { NULL };

/*
  Plugin library descriptor
*/

mysql_declare_plugin(bioparser)
{
  MYSQL_FTPARSER_PLUGIN,      /* type                            */
  &bioparser_descriptor,  /* descriptor                      */
  "bioparser",            /* name                            */
  "Pierre Lindenbaum PhD",                 /* author                          */
  "finds rs### ids.",  /* description                     */
  PLUGIN_LICENSE_GPL,
  bioparser_plugin_init,  /* init function (when loaded)     */
  bioparser_plugin_deinit,/* deinit function (when unloaded) */
  0x0001,                     /* version                         */
  simple_status,              /* status variables                */
  simple_system_variables,    /* system variables                */
  NULL
}
mysql_declare_plugin_end;

/*  libticalcs - calculator library, a part of the TiLP project
 *  Copyright (C) 1999-2002  Romain Lievin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "calc_err.h"
#include "calc_ext.h"
#include "defs92p.h"
#include "keys92p.h"
#include "group.h"
#include "pause.h"
#include "rom89.h"
#include "update.h"
//#include "struct.h"

#ifdef HAVE_CURSES_H
#include <curses.h>
#endif

#ifndef __WIN32__
static uint32_t bswap_32(uint32_t a)
{
  return (a >> 24) | ((a & 0xff0000) >> 16) << 8 | ((a & 0xff00) >> 8) << 16 | (a & 0xff) << 8;
}
#else
static DWORD bswap_32(DWORD a)
{
  return (a >> 24) | ((a & 0xff0000) >> 16) << 8 | ((a & 0xff00) >> 8) << 16 | (a & 0xff) << 8;
}
#endif

/* Functions used by TI_PC functions */

// The PC indicates that is OK
int PC_replyOK_92p(void)
{
  TRY(cable->put(PC_TI92p));
  TRY(cable->put(CMD92p_TI_OK));
  TRY(cable->put(0x00));
  TRY(cable->put(0x00));
  DISPLAY("The computer reply OK.\n");

  return 0;
}

int PC_replyCONT_92p(void)
{
  TRY(cable->put(PC_TI92p));
  TRY(cable->put(CMD92p_CONTINUE));
  TRY(cable->put(0x00));
  TRY(cable->put(0x00));
  DISPLAY("The computer reply OK.\n");

  return 0;
}

// The PC indicates that it is ready or wait data
int PC_waitdata_92p(void)
{
  TRY(cable->put(PC_TI92p));
  TRY(cable->put(CMD92p_WAIT_DATA));
  TRY(cable->put(0x00));
  TRY(cable->put(0x00));
  DISPLAY("The computer wait data.\n");
  
  return 0;
}

// Check whether the TI reply OK
int ti92p_isOK(void)
{
  byte data;

  TRY(cable->get(&data));
  if(data != TI92p_PC) 
    {
      return ERR_NOT_REPLY;
    }
  TRY(cable->get(&data));
  if(data != CMD92p_TI_OK)
    { 
      if(data == CMD92p_CHK_ERROR) return ERR_CHECKSUM;
      else return ERR_NOT_REPLY;
    }
  TRY(cable->get(&data));
  if(data != 0x00)
    { 
      // FLASHed calcs reply in a different way
      //return 4;
    }
  TRY(cable->get(&data));
  if((data&1) != 0)
    { 
      return ERR_NOT_REPLY;
    }

  DISPLAY("The calculator reply OK.\n");

  return 0;
}

// The TI indicates that it is ready or wait data
int ti92p_waitdata(void)
{
  byte data;

  TRY(cable->get(&data));
  if(data != TI92p_PC) return ERR_INVALID_BYTE;
  TRY(cable->get(&data));
  if(data != CMD92p_WAIT_DATA)
    {
      if(data != CMD92p_REFUSED)
        {
          DISPLAY("Data: %02X\n", data);
          return ERR_INVALID_BYTE;
        }
      else
        {
          DISPLAY("Command rejected... ");  
          TRY(cable->get(&data));
          TRY(cable->get(&data));
          TRY(cable->get(&data));
          //*rej_code = data;
          DISPLAY("Rejection code: %02X\n", data);
          TRY(cable->get(&data));
          TRY(cable->get(&data));
          return ERR_VAR_REFUSED;
        }
    }
  TRY(cable->get(&data));
  TRY(cable->get(&data));
  DISPLAY("The calculator wait data.\n");

  return 0;
}

// Check whether the TI reply that it is ready
int ti92p_isready(void)
{
  TRY(cable->open());
  DISPLAY("Is calculator ready ?\n");
  TRY(cable->put(PC_TI92p));
  TRY(cable->put(CMD92p_ISREADY));
  TRY(cable->put(0x00));
  TRY(cable->put(0x00));
  TRY(ti92p_isOK());
  DISPLAY("The calculator is ready.\n");
  DISPLAY("\n");
  TRY(cable->close());

  return 0;
}

// Send a string of characters to the TI
int ti92p_sendstring(char *s, word *checksum)
{
  int i;

  for(i=0; i<strlen(s); i++)
    {
      TRY(cable->put(s[i]));
      (*checksum) += (s[i] & 0xff);
    }

  return 0;
}

#define TI92p_MAXTYPES 48
const char *TI92p_TYPES[TI92p_MAXTYPES]=
{ 
"EXPR", "UNKNOWN", "UNKNOWN", "UNKNOWN", "LIST", "UNKNOWN", "MAT", "UNKNOWN", 
"UNKNOW", "UNKNOWN", "DATA", "TEXT", "STR", "GDB", "FIG", "UNKNOWN",
"PIC", "UNKNOWN", "PRGM", "FUNC", "MAC", "UNKNOWN", "UNKNOWN", "UNKNOWN",
"UNKNOWN", "UNKNOWN", "UNKNOWN", "UNKNOWN", "ZIP", "BACKUP", "UNKNOWN", "DIR",
"UNKNOW", "ASM", "IDLIST", "UNKNOWN", "FLASH", "UNKNOWN", "LOCKED", "ARCHIVED",
"UNKNOWN", "UNKNOWN", "UNKNOWN", "UNKNOWN", "UNKNOWN", "UNKNOWN", "UNKNOWN", "UNKNOWN"
};
const char *TI92p_EXT[TI92p_MAXTYPES]=
{
"9Xe", "unknown", "unknown", "unknown", "9Xl", "unknown", "9Xm", "unknown",
"unknown", "unknown", "9Xc", "9Xt", "9Xs", "9Xd", "9Xa", "unknown",
"9Xi", "unknown", "9Xp", "9Xf", "9Xx", "unknown", "unknown", "unknown", 
"unknown", "unknown", "unknown", "unknown", "9Xy", "9Xb", "unknown", "unknown",
"unknown", "9Xz", "unknown", "unknown", "9Xk", "unknown", "unknown", "unknown",
"unknown", "unknown", "unknown", "unknown", "unknown", "unknown", "unknown", "unknown"
};

// Return the type corresponding to the value
const char *ti92p_byte2type(byte data)
{
  if(data>TI92p_MAXTYPES)
    {
      printf("Type: %02X\n", data);
      printf("Warning: unknown type. It is a bug. Please report this information.\n");
      return "UNKNOWN";
    }
  else 
    {
      return TI92p_TYPES[data];
    }
}

// Return the value corresponding to the type
byte ti92p_type2byte(char *s)
{
  int i;

  for(i=0; i<TI92p_MAXTYPES; i++)
    {
      if(!strcmp(TI92p_TYPES[i], s)) break;
    }
  if(i>TI92p_MAXTYPES)
    {
      printf("Warning: unknown type. It is a bug. Please report this information.\n");
      return 0;
    }

  return i;
}

// Return the file extension corresponding to the value
const char *ti92p_byte2fext(byte data)
{
  if(data>TI92p_MAXTYPES)
    {
      printf("Type: %02X\n", data);
      printf("Warning: unknown type. It is a bug. Please report this information.\n");    
      return ".92p?";
    }
  else 
  {
    return TI92p_EXT[data];
  }
}

// Return the value corresponding to the file extension
byte ti92p_fext2byte(char *s)
{
  int i;

  for(i=0; i<TI92p_MAXTYPES; i++)
    {
      if(!strcmp(TI92p_EXT[i], s)) break;
    }
  if(i > TI92p_MAXTYPES)
    {
      printf("Warning: unknown type. It is a bug. Please report this information.\n");
      return 0;
    }

  return i;
}

// General functions

int ti92p_send_key(word key)
{
  TRY(cable->open());
  TRY(cable->put(PC_TI92p));
  TRY(cable->put(CMD92p_DIRECT_CMD));
  TRY(cable->put(LSB(key)));
  TRY(cable->put(MSB(key)));
  TRY(ti92p_isOK());
  TRY(cable->close());

  return 0;
}

int ti92p_remote_control(void)
{
#if defined(HAVE_CURSES_H) && defined(HAVE_LIBCURSES)
  int c;
  word d;
  int sp_key;
  char skey[10];
  int b;

  TRY(cable->open());
  d=0;
  DISPLAY("\n");
  DISPLAY("Remote control: press any key but for:\n");
  DISPLAY("2nd, press the square key\n");
  DISPLAY("diamond, press the tab key\n");
  DISPLAY("APPS, press F9\n");
  DISPLAY("STO, press F10\n");
  DISPLAY("MODE, press F11\n");
  DISPLAY("CLEAR, press F12\n");
  DISPLAY("Press End to quit the remote control mode.\n");
  getchar();
  initscr();
  noecho();
  keypad(stdscr, TRUE);
  raw();
  do
    {
      sp_key=0;
      b=0;
      strcpy(skey, "");
      c=getch();
      if(c==9)
	{
	  sp_key=KEY92p_CTRL;
	  strcpy(skey, "DIAMOND ");
	  b=1;
	  c=getch();
	}
      if(c==178)
        {
          sp_key=KEY92p_2ND;
	  strcpy(skey, "2nd ");
	  b=1;
	  c=getch();
        }
      if(c>31 && c<128)
	{
	  DISPLAY("Sending <%s%c>\n", skey, c);
	  TRY(cable->put(PC_TI92p));
	  TRY(cable->put(CMD92p_DIRECT_CMD));
	  c+=sp_key;
	  DISPLAY("%i\n", c);
	  TRY(cable->put(LSB(c)));
	  TRY(cable->put(MSB(c)));
	  TRY(ti92p_isOK());
	  refresh();
	}
      else
	{
	  if(c==ESC)
            {
              d=KEY92p_ESC;
              DISPLAY("Sending %sESC key\n", skey);
            }
	  if(c==BS)
            {
              d=KEY92p_BS;
              DISPLAY("Sending %s<- key\n", skey);
            }
	  if(c==F1) 
	    {
	      d=KEY92p_F1;
	      DISPLAY("Sending %sF1 key\n", skey);	      
	    }
	  if(c==F2) 
	    {
	      d=KEY92p_F2;
	      DISPLAY("Sending %sF2 key\n", skey);
	    }
          if(c==F3) 
	    {
	      d=KEY92p_F3;
	      DISPLAY("Sending %sF3 key\n", skey);
	    }
          if(c==F4) 
	    {
	      d=KEY92p_F4;
	      DISPLAY("Sending %sF4 key\n", skey);
	    }
          if(c==F5) 
	    {
	      d=KEY92p_F5;
	      DISPLAY("Sending %sF5 key\n", skey);
	    }
          if(c==F6) 
	    {
	      d=KEY92p_F6;
	      DISPLAY("Sending %sF6 key\n", skey);
	    }
	  if(c==F7) 
	    {
	      d=KEY92p_F7;
	      DISPLAY("Sending %sF7 key\n", skey);
	    }
          if(c==F8) 
	    {
	      d=KEY92p_F8;
	      DISPLAY("Sending %sF8 key\n", skey);
	    }
          if(c==F9) 
	    {
	      d=KEY92p_APPS;
	      DISPLAY("Sending %sAPPS key\n", skey);
	    }
          if(c==F10) 
	    {
	      d=KEY92p_STO;
	      DISPLAY("Sending %sSTO key\n", skey);
	    }
          if(c==F11) 
	    {
	      d=KEY92p_MODE;
	      DISPLAY("Sending %sMODE key\n", skey);
	    }
          if(c==F12) 
	    {
	      d=KEY92p_CLEAR;
	      DISPLAY("Sending %sCLEAR key\n", skey);
	    }
	  if(c==CALC_CR || c==CALC_LF) 
	    {
	      d=KEY92p_CR;
	      DISPLAY("Sending %sENTER key\n", skey);
	    }
	  d+=sp_key;
	  TRY(cable->put(PC_TI92p));
          TRY(cable->put(CMD92p_DIRECT_CMD));
          TRY(cable->put(LSB(d)));
          TRY(cable->put(MSB(d)));
          TRY(ti92p_isOK());
	  refresh();
	}
    }
  while(c!=END);
  noraw();
  endwin();
  TRY(cable->close());
  
  return 0;
#else  
  return ERR_VOID_FUNCTION;
#endif
}

int ti92p_screendump(byte **bitmap, int mask_mode,
                         struct screen_coord *sc)
{
  byte data;
  word max_cnt;
  word sum;
  word checksum;
  int i;

  TRY(cable->open());

  update->start();
  sc->width=TI92p_COLS;
  sc->height=TI92p_ROWS;
  sc->clipped_width=TI92p_COLS;
  sc->clipped_height=TI92p_ROWS;
  if(*bitmap != NULL)
	  free(*bitmap);
  (*bitmap)=(byte *)malloc(TI92p_COLS*TI92p_ROWS*sizeof(byte)/8);
  if((*bitmap) == NULL)
    {
      fprintf(stderr, "Unable to allocate memory.\n");
      exit(0);
    }

  sum=0;
  DISPLAY("Requesting screendump.\n");
  TRY(cable->put(PC_TI92p));
  TRY(cable->put(CMD92p_SCREEN_DUMP));
  TRY(cable->put(0x00));
  TRY(cable->put(0x00));
  TRY(ti92p_isOK());

  TRY(cable->get(&data));
  if(data != TI92p_PC) return 6;
  TRY(cable->get(&data));
  if(data != CMD92p_DATA_PART) return 6;  
  TRY(cable->get(&data));
  max_cnt=data;
  TRY(cable->get(&data));
  max_cnt += (data << 8);
  DISPLAY("0x%04X = %i bytes to receive\n", max_cnt, max_cnt);
  DISPLAY("Screendump in progress...\n");

  update->total = max_cnt;
  for(i=0; i<max_cnt; i++)
    {
      TRY(cable->get(&data));
      (*bitmap)[i]=~data;
      sum += data;

      update->count = i;
      update->percentage = (float)i/max_cnt;
      update->pbar();
      if(update->cancel) return -1;
    }
  
  TRY(cable->get(&data));
  checksum=data;
  TRY(cable->get(&data));
  checksum += (data << 8);
  //if(sum != checksum) return 7;
  //DISPLAY("Ckechsum: %04X\n", checksum);
  //DISPLAY("Sum: %04X\n", sum);
  //DISPLAY("Checksum OK.\n");

  TRY(cable->put(PC_TI92p));
  TRY(cable->put(CMD92p_PC_OK));
  TRY(cable->put(0x00));
  TRY(cable->put(0x00));
  DISPLAY("PC reply OK.\n");

  update->start();
  TRY(cable->close());

  return 0;
}

int ti92p_directorylist(struct varinfo *list, int *n_elts)
{
  byte data;
  word sum;
  word checksum;
  int i, j;
  word block_size;
  byte var_type;
  byte locked;
  longword var_size;
  byte name_length;
  char var_name[9];
  struct varinfo *p, *f;
  struct varinfo *q, *tmp;
  byte num_var;


  TRY(cable->open());
  update_start();
  *n_elts=0;
  p=list;
  f=NULL;
  p->folder=f;
  p->is_folder = VARIABLE;
  p->next=NULL;
  p->folder=NULL;
  strcpy(p->varname, "");
  p->varsize=0;
  p->vartype=0;
  p->varattr=0;
  strcpy(p->translate, "");

  sum=0;
  DISPLAY("Requesting directory list (dir)...\n");
  TRY(cable->put(PC_TI92p));
  TRY(cable->put(CMD92p_REQUEST));
  TRY(cable->put(0x06));
  TRY(cable->put(0x00));
  for(i=0; i<3; i++)
    {
      TRY(cable->put(0x00));
    }
  data=TI92p_DIR;
  sum+=data;
  TRY(cable->put(data));
  data=TI92p_DIRL;
  sum+=data;
  TRY(cable->put(data));
  TRY(cable->put(0x00));
  TRY(cable->put(LSB(sum)));
  TRY(cable->put(MSB(sum)));

  TRY(ti92p_isOK());
  sum=0;
  TRY(cable->get(&data));
  if(data != TI92p_PC) return ERR_INVALID_BYTE;
  TRY(cable->get(&data));
  if(data != CMD92p_VAR_HEADER) return ERR_INVALID_BYTE;
  TRY(cable->get(&data));
  TRY(cable->get(&data));
  TRY(cable->get(&data));
  var_size=data;
  sum+=data;
  TRY(cable->get(&data));
  var_size |= (data << 8);
  sum+=data;
  TRY(cable->get(&data));
  var_size |= (data << 16);
  sum+=data;
  TRY(cable->get(&data));
  var_size |= (data << 24);
  sum+=data;
  TRY(cable->get(&data));
  list->vartype=data;
  sum+=data;
  TRY(cable->get(&data));
  name_length=data;
  sum+=data;
  for(i=0; i<name_length; i++)
    {
      TRY(cable->get(&data));
      var_name[i]=data;
      sum+=data;
    }
  var_name[i]='\0';
  strcpy(list->varname, var_name);
  strncpy(list->translate, list->varname, 9);
  list->folder=NULL;
  TRY(cable->get(&data));
  sum+=data;
  TRY(cable->get(&data));
  checksum=data;
  TRY(cable->get(&data));
  checksum += (data << 8);
  if(checksum != sum) return ERR_CHECKSUM;
  TRY(PC_replyOK_92p());
  TRY(PC_waitdata_92p());

  TRY(ti92p_isOK());
  TRY(cable->get(&data));
  if(data != TI92p_PC) return ERR_INVALID_BYTE;
  TRY(cable->get(&data));
  if(data != CMD92p_DATA_PART) return ERR_INVALID_BYTE;
  TRY(cable->get(&data));
  block_size=data;
  TRY(cable->get(&data));
  block_size += (data << 8);
  sum=0;
  for(i=0; i<4; i++)
    {
      TRY(cable->get(&data));
      sum+=data;
    }
  block_size=block_size-4;
  for(j=0; j<block_size/14; j++)
    {
      if( (p->next=(struct varinfo *)malloc(sizeof(struct varinfo))) == NULL)
        {
          fprintf(stderr, "Unable to allocate memory.\n");
          exit(0);
        }
      p=p->next;
      p->next=NULL;
      (*n_elts)++;
      //strcpy(p->varname, "");
      p->is_folder = FOLDER;

      for(i=0; i<8; i++)
        {
          TRY(cable->get(&data));
          sum+=data;
          var_name[i]=data;
        }
      var_name[i]='\0';      
      strcpy(p->varname, var_name);
      strncpy(p->translate, p->varname, 9);
      TRY(cable->get(&data));
      var_type=data;
      sum+=data;
      p->vartype=var_type;
      if(p->vartype == TI92p_FLASH)
        p->is_folder = VARIABLE;
      TRY(cable->get(&data));
      locked=data;
      sum+=data;
      p->varattr=locked;
      TRY(cable->get(&data));
      var_size=data;
      sum+=data;
      TRY(cable->get(&data));
      var_size |= (data << 8);
      sum+=data;
      TRY(cable->get(&data));
      var_size |= (data << 16);
      sum+=data;
      TRY(cable->get(&data));
      // See the doc: PROTOCOL.92p
      //      var_size |= (data << 24);
      sum+=data;
      p->varsize=var_size;
      DISPLAY("Name: %8s | ", var_name);
      DISPLAY("Type: %8s | ", ti92p_byte2type(var_type));
      DISPLAY("Locked: %i | ", locked);
      DISPLAY("Size: %08X\n", var_size);
      p->folder=p;
    }
  TRY(cable->get(&data));
  checksum=data;
  TRY(cable->get(&data));
  checksum += (data << 8);
  if(checksum != sum) return ERR_CHECKSUM;

  TRY(PC_replyOK_92p());
  sprintf(update->label_text, "Reading of directory: %s", 
	   p->translate);
  update->label();
  if(update->cancel) return ERR_ABORT;
  
  TRY(cable->get(&data));
  if(data != TI92p_PC) return ERR_INVALID_BYTE;
  TRY(cable->get(&data));
  TRY(cable->get(&data));
  TRY(cable->get(&data));
  DISPLAY("The calculator does not want to continue.\n");

  TRY(PC_replyOK_92p());

  ////////

  q=list->next;
  do
    {
      tmp=q->next;
      if(q->vartype == TI92p_FLASH) {
        q=tmp;
        continue;
      }

      DISPLAY("Requesting local directory list in %8s...\n", q->varname);
      p=q;
      num_var=0;
      sum=0;
      TRY(cable->put(PC_TI92p));
      TRY(cable->put(CMD92p_REQUEST));
      block_size=4+2+strlen(q->varname);
      TRY(cable->put(LSB(block_size)));
      TRY(cable->put(MSB(block_size)));
      for(i=0; i<3; i++)
	{
	  TRY(cable->put(0x00));
	}
      data=TI92p_LDIR;
      sum+=data;
      TRY(cable->put(data));
      data=TI92p_DIRL;
      sum+=data;
      TRY(cable->put(data));
      data=strlen(q->varname);
      sum+=data;
      TRY(cable->put(data));
      TRY(ti92p_sendstring(q->varname, &sum));
      TRY(cable->put(LSB(sum)));
      TRY(cable->put(MSB(sum)));

      TRY(ti92p_isOK());

      sum=0;
      TRY(cable->get(&data));
      if(data != TI92p_PC) return ERR_INVALID_BYTE;
      TRY(cable->get(&data));
      if(data != CMD92p_VAR_HEADER) return ERR_INVALID_BYTE;
      TRY(cable->get(&data));
      TRY(cable->get(&data));
      TRY(cable->get(&data));
      var_size=data;
      sum+=data;
      TRY(cable->get(&data));
      var_size |= (data << 8);
      sum+=data;
      TRY(cable->get(&data));
      var_size |= (data << 16);
      sum+=data;
      TRY(cable->get(&data));
      var_size |= (data << 24);
      sum+=data;
      TRY(cable->get(&data));
      sum+=data;
      TRY(cable->get(&data));
      name_length=data;
      sum+=data;
      for(i=0; i<name_length; i++)
	{
	  TRY(cable->get(&data));
	  var_name[i]=data;
	  sum+=data;
	}
      var_name[i]='\0';
      TRY(cable->get(&data));
      sum+=data;
      TRY(cable->get(&data));
      checksum=data;
      TRY(cable->get(&data));
      checksum += (data << 8);
      if(checksum != sum) return ERR_CHECKSUM;
      TRY(PC_replyOK_92p());
      TRY(PC_waitdata_92p());

      TRY(ti92p_isOK());
      TRY(cable->get(&data));
      if(data != TI92p_PC) return ERR_INVALID_BYTE;
      TRY(cable->get(&data));
      if(data != CMD92p_DATA_PART) return ERR_INVALID_BYTE;
      TRY(cable->get(&data));
      block_size=data;
      TRY(cable->get(&data));
      block_size += (data << 8);
      sum=0;
      for(i=0; i<4; i++)
	{
	  TRY(cable->get(&data));
	  sum+=data;
	}
      block_size-=4;
      for(i=0; i<14; i++)
        {
          TRY(cable->get(&data));
          sum+=data;
        }
      block_size-=14;
      for(j=0; j<block_size/14; j++)
	{ 
	  TicalcVarInfo *old = q;
	  if( (q->next=(struct varinfo *)malloc(sizeof(struct varinfo))) == NULL)
	    {
	      fprintf(stderr, "Unable to allocate memory.\n");
	      exit(0);
	    }
	  q=q->next;
	  (*n_elts)++;
	  num_var++;

	  q->is_folder = VARIABLE;
	  
	  for(i=0; i<8; i++)
	    {
	      TRY(cable->get(&data));
	      sum+=data;
	      var_name[i]=data;
	    }
	  var_name[i]='\0';
	  strcpy(q->varname, var_name);
	  strncpy(q->translate, q->varname, 9);
	  TRY(cable->get(&data));
	  var_type=data;
	  sum+=data;
	  q->vartype=var_type;
	  TRY(cable->get(&data));
	  locked=data;
	  sum+=data;
	  q->varattr=locked;
	  TRY(cable->get(&data));
	  var_size=data;
	  sum+=data;
	  TRY(cable->get(&data));
	  var_size |= (data << 8);
	  sum+=data;
	  TRY(cable->get(&data));
	  var_size |= (data << 16);
	  sum+=data;
	  TRY(cable->get(&data));
	  // See the doc: PROTOCOL.92p
	  //	  var_size |= (data << 24);
	  sum+=data;
	  q->varsize=var_size;

	  /* if FLASH type, discards it */
          if(q->vartype == TI92p_FLASH) {
            free(q);
            q = old;
            continue;
          }

	  DISPLAY("Name: %8s | ", var_name);
	  DISPLAY("Type: %8s | ", ti92p_byte2type(var_type));
	  DISPLAY("Attr: %i | ", locked);
	  DISPLAY("Size: %08X\n", var_size);
	  list->varsize += var_size;
	  q->folder=p;
	  sprintf(update->label_text, "Reading of: %s/%s", 
		   (q->folder)->translate, q->translate);
	  update->label();
	  if(update->cancel) return ERR_ABORT;
	}
      TRY(cable->get(&data));
      checksum=data;
      TRY(cable->get(&data));
      checksum += (data << 8);
      if(checksum != sum) return ERR_CHECKSUM;
      
      TRY(PC_replyOK_92p());
      
      TRY(cable->get(&data));
      if(data != TI92p_PC) return ERR_INVALID_BYTE;
      TRY(cable->get(&data));
      TRY(cable->get(&data));
      TRY(cable->get(&data));
      DISPLAY("The calculator do not want continue.\n");
      
      TRY(PC_replyOK_92p());

      q->next=tmp;
      q=tmp;
      p->varsize=num_var;
    }
  while(q != NULL);
  DISPLAY("\n");
  TRY(cable->close());


  return 0;
}

int ti92p_recv_var(FILE *file, int mask_mode, 
		   char *varname, byte vartype, byte varlock)
{
  byte data;
  word sum;
  word checksum;
  word block_size;
  int i;
  longword var_size;
  byte name_length;
  char name[9];

  TRY(cable->open());
  update->start();
  sprintf(update->label_text, "Variable: %s", varname);
  update->label();
  sum=0;
  DISPLAY("Request variable: %s\n", varname);
  TRY(cable->put(PC_TI92p));
  TRY(cable->put(CMD92p_REQUEST));
  block_size=4+2+strlen(varname);
  data=LSB(block_size);
  TRY(cable->put(data));
  data=MSB(block_size);
  TRY(cable->put(data));
  for(i=0; i<4; i++)
    {
      TRY(cable->put(0x00));
    }
  data=vartype;
  TRY(cable->put(data));
  sum+=data;
  data=strlen(varname);
  TRY(cable->put(data));
  sum+=data;
  TRY(ti92p_sendstring(varname, &sum));
  TRY(cable->put(LSB(sum)));
  TRY(cable->put(MSB(sum)));
  
  TRY(ti92p_isOK());
  sum=0;
  TRY(cable->get(&data));
  if(data != TI92p_PC) return ERR_INVALID_BYTE;
  TRY(cable->get(&data));
  if(data != CMD92p_VAR_HEADER) return ERR_PACKET;
  TRY(cable->get(&data));
  TRY(cable->get(&data));
  TRY(cable->get(&data));
  var_size=data;
  sum+=data;
  TRY(cable->get(&data));
  var_size |= (data << 8);
  sum+=data;
  TRY(cable->get(&data));
  var_size |= (data << 16);
  sum+=data;
  TRY(cable->get(&data));
  var_size |= (data << 24);
  sum+=data;
  DISPLAY("Size of the var in memory: 0x%08X = %i.\n", var_size-2, var_size-2);
  TRY(cable->get(&data));
  DISPLAY("Type of the variable: %s\n", ti92p_byte2type(data));
  sum+=data;
  TRY(cable->get(&data));
  name_length=data;
  sum+=data;
  DISPLAY("Variable name: ");
  for(i=0; i<name_length; i++)
    {
      TRY(cable->get(&data));
      name[i]=data;
      DISPLAY("%c", data);
      sum+=data;
    }
  name[i]='\0';
  DISPLAY("\n");
  TRY(cable->get(&data)); // It's the only difference with the 92p
  sum+=data;
  TRY(cable->get(&data));
  checksum=data;
  TRY(cable->get(&data));
  checksum += (data << 8);
  if(checksum != sum) return ERR_CHECKSUM;

  TRY(PC_replyOK_92p());
  TRY(PC_waitdata_92p());
  DISPLAY("The calculator want continue.\n");

  TRY(ti92p_isOK());
  DISPLAY("Receiving variable...\n");
  TRY(cable->get(&data));
  if(data != TI92p_PC) return ERR_INVALID_BYTE;
  TRY(cable->get(&data));
  if(data != CMD92p_DATA_PART) return ERR_INVALID_BYTE;
  TRY(cable->get(&data));
  TRY(cable->get(&data));
  sum=0;
  for(i=0; i<4; i++)
    {
      TRY(cable->get(&data));
      sum+=data;
      fprintf(file, "%c", data);
    }
  TRY(cable->get(&data));
  block_size = (data << 8);
  sum+=data;
  fprintf(file, "%c", data);
  TRY(cable->get(&data));
  block_size |= data;
  sum+=data;
  fprintf(file, "%c", data);
  update->total = block_size;
  for(i=0; i<block_size; i++)
    {
      TRY(cable->get(&data));
      sum+=data;
      fprintf(file, "%c", data);

      update->count = i;
      update->percentage = (float)i/block_size;
      update->pbar();
      if(update->cancel) return -1;
    }
  TRY(cable->get(&data));
  checksum=data;
  fprintf(file, "%c", data);
  TRY(cable->get(&data));
  checksum += (data << 8);
  fprintf(file, "%c", data);
  if(checksum != sum) return ERR_CHECKSUM;
  TRY(PC_replyOK_92p());
  TRY(cable->get(&data));
  if(data != TI92p_PC) return ERR_INVALID_BYTE;
  TRY(cable->get(&data));
  DISPLAY("The calculator do not want continue.\n");
  TRY(cable->get(&data));
  TRY(cable->get(&data));

  TRY(cable->put(PC_TI92p));
  TRY(cable->put(CMD92p_TI_OK));
  TRY(cable->put(0x00));
  TRY(cable->put(0x00));
  DISPLAY("\n");

  update->start();
  TRY(cable->close());
  PAUSE(PAUSE_BETWEEN_VARS);

  return 0;
}

int ti92p_send_var(FILE *file, int mask_mode)
{
  byte data;
  word sum;
  word block_size = 0;
  int i;
  int j;
  int k;
  longword varsize;
  byte vartype;
  char foldname[18];
  char varname[18];
  byte varattr;
  char str[128];
  int num_vars_in_folder;
  int num_entries;
  int var_index;
  static int num_vars;
  static char **t_varname=NULL;
  static byte *t_vartype=NULL;
  static struct varinfo dirlist;
  int n;
  int err;
  int exist=0;
  int action=ACTION_NONE;
  static int do_dirlist=1;

  if((mask_mode & MODE_DIRLIST) && do_dirlist) // do dirlist one time
    {
      TRY(ti92p_directorylist(&dirlist, &n));
      do_dirlist=0;
    }

  if((mask_mode & MODE_SEND_LAST_VAR) ||
     (mask_mode & MODE_SEND_ONE_VAR)) do_dirlist=1;

  // it seems that TIGLv2.00 use always 0xC9
  //mask_mode |= MODE_USE_2ND_HEADER;
  if(t_vartype != NULL) { free(t_vartype); t_vartype=NULL; }
  if(t_varname != NULL)
    {
      for(i=0; i<num_vars; i++) free(t_varname[i]); 
      t_varname=NULL;
      num_vars=0;
    }
  num_vars=0;
  var_index=0;
  TRY(cable->open());
  update->start();
  fgets(str, 9, file);
  if(!(mask_mode & MODE_FILE_CHK_NONE))
    {
      if(mask_mode & MODE_FILE_CHK_MID)
	{
	  if( strcmp(str, "**TI92P*") && strcmp(str, "**TI92p*") &&
	      strcmp(str, "**TI89**") && strcmp(str, "**TI92**") )
	    { 
	      return ERR_INVALID_TIXX_FILE;
	    }
	}
      else if(mask_mode & MODE_FILE_CHK_ALL)
	{
	  fprintf(stderr, "MODE_FILE_CHK_ALL\n");
	  if( strcmp(str, "**TI89**"))
	    {
	      return ERR_INVALID_TI89_FILE;
	    }
	}
    }

  for(i=0; i<2; i++) fgetc(file);
  for(i=0; i<8; i++) foldname[i]=fgetc(file);
  foldname[i]='\0';
  for(i=0; i<40; i++) fgetc(file);
  num_entries=fgetc(file);
  num_entries+=fgetc(file) << 8;

  if(num_entries == 1)
    { // normal file, not a group file
      t_varname=(char **)malloc(sizeof(char *));
      if(t_varname == NULL) return 40; 
      t_varname[0]=(char *)malloc(18*sizeof(char));
      if(t_varname[0] == NULL) return 40;
      t_vartype=(byte *)malloc(sizeof(byte));
      if(t_vartype == NULL) return 40; 
      num_vars=1;
      for(i=0; i<4; i++) fgetc(file);
      for(i=0; i<8; i++) t_varname[var_index][i]=fgetc(file);
      t_varname[var_index][i]='\0';
      if(! (mask_mode & MODE_LOCAL_PATH))
	{ // full path
	  strcat(foldname, "\\");
	  strcat(foldname, t_varname[var_index]);
	  strcpy(t_varname[var_index], foldname);
	}
      t_vartype[var_index]=fgetc(file);
      fgetc(file);
      for(i=0; i<2; i++) fgetc(file);
    }
  else
  { // group file
    k=0;
    while(k<num_entries)
      {
	k++;
	for(i=0; i<4; i++) fgetc(file);
	for(i=0; i<8; i++) foldname[i]=fgetc(file);
	foldname[i]='\0';
	vartype=fgetc(file);
	fgetc(file);
	num_vars_in_folder=fgetc(file);
	num_vars_in_folder+=fgetc(file) << 8;
	num_vars+=num_vars_in_folder;
        t_vartype=(byte *)realloc(t_vartype, (num_vars+1)*sizeof(byte));
        if(t_vartype == NULL) return 40;
	t_varname=(char **)realloc(t_varname, (num_vars+1)*sizeof(char *));
	if(t_varname == NULL) return 40;

	for(j=0; j<num_vars_in_folder; j++)
	  { 
	    t_varname[var_index]=(char *)malloc(18*sizeof(char));
	    if(t_varname[var_index] == NULL) return 40; 	
	    k++;
	    for(i=0; i<4; i++) fgetc(file);
	    for(i=0; i<8; i++) varname[i]=fgetc(file);
	    varname[i]='\0';
	    vartype=fgetc(file);
	    t_vartype[var_index]=vartype;
	    varattr=fgetc(file);
	    if( (mask_mode & MODE_USE_2ND_HEADER)    // if backup
		|| (mask_mode & MODE_KEEP_ARCH_ATTRIB) ) // or if use the extended file format
	      {
		switch(varattr)
		  {
		  case TI92p_VNONE: t_vartype[var_index]=TI92p_BKUP;
		    break;
		  case TI92p_VLOCK: t_vartype[var_index]=0x26;
		    break;
		  case TI92p_VARCH: t_vartype[var_index]=0x27;
		    break;
		  }
	      }
	    for(i=0; i<2; i++) fgetc(file);
	    if(! (mask_mode & MODE_LOCAL_PATH))
	      {
		strcpy(t_varname[var_index], foldname);
		strcat(t_varname[var_index], "\\");
		strcat(t_varname[var_index], varname); 	    
	      }
	    else
	      strcpy(t_varname[var_index], varname);
	    var_index++;
	  }
      }
    var_index=0;
  }
  for(i=0; i<4; i++) fgetc(file);
  for(i=0; i<2; i++) fgetc(file);
  /*
  for(i=0; i<num_vars; i++)
    {
      printf("-> <%8s> <%02X>\n", t_varname[i], t_vartype[i]);
    }
  */
  for(j=0; j<num_vars; j++, var_index++)
    {
      if(mask_mode & MODE_USE_2ND_HEADER)
	{
	  (update->main_percentage)=(float)j/num_vars;
	  update->pbar();
	}
      for(i=0; i<4; i++) fgetc(file);
      varsize=fgetc(file) << 8;
      varsize+=fgetc(file);
      varsize+=2;
      
      sprintf(update->label_text, "Variable: %s", 
	       t_varname[var_index]);
      update->label();
      DISPLAY("Sending variable...\n");
      DISPLAY("Name: %s\n", t_varname[var_index]);
      DISPLAY("Size: %08X\n", varsize-2);
      DISPLAY("Type: %s\n", ti92p_byte2type(t_vartype[var_index]));

      /**/
      exist=check_if_var_exist(&dirlist, t_varname[var_index]);
      if(exist && (mask_mode & MODE_DIRLIST))
        {
          action=update->choose(t_varname[var_index], varname);
          if(!strcmp(varname, "") && (action==ACTION_RENAME)) 
	    action=ACTION_SKIP;
          switch(action)
            {
            case ACTION_SKIP: // skip var
	      DISPLAY("User action: skip.\n");
              for(i=0; i<varsize; i++)
                data=fgetc(file);
              continue;
              break;
            case ACTION_OVERWRITE: //try to overwrite it
              DISPLAY("User action: overwrite.\n");
              break;
            case ACTION_RENAME: //rename var
              DISPLAY("User action: rename from %s into %s.\n",
                      t_varname[var_index], varname);
              strcpy(t_varname[var_index], varname);
              break;
            default: // skip
	      for(i=0; i<block_size+2; i++)
                data=fgetc(file);
              continue;
              break;
            }
        }
      /**/

      sum=0;
      TRY(cable->put(PC_TI92p));
      if((mask_mode & MODE_USE_2ND_HEADER))
	{ // used for send backup
	  TRY(cable->put(CMD92p_VAR_HEADER2));
	}
      else
	{ // used for normal files
	  TRY(cable->put(CMD92p_VAR_HEADER));
	}
      block_size=4+2+strlen(t_varname[var_index])+1; // difference: see comment below
      TRY(cable->put(LSB(block_size)));
      TRY(cable->put(MSB(block_size)));
      data=varsize & 0x000000FF;
      sum+=data;
      TRY(cable->put(data));
      data=(varsize & 0x0000FF00) >> 8;
      sum+=data;
      TRY(cable->put(data));
      data=(varsize & 0x00FF0000) >> 16;
      sum+=data;
      TRY(cable->put(data));
      data=(varsize & 0xFF000000) >> 24;
      sum+=data;
      TRY(cable->put(data));
      data=t_vartype[var_index];
      sum+=data;
      TRY(cable->put(data));
      data=strlen(t_varname[var_index]);
      sum+=data;
      TRY(cable->put(data));
      TRY(ti92p_sendstring(t_varname[var_index], &sum));
      data=0x00;
      TRY(cable->put(data)); // The unique difference in comparison with the 92
      TRY(cable->put(LSB(sum)));
      TRY(cable->put(MSB(sum)));
      
      TRY(ti92p_isOK());
      TRY(ti92p_waitdata());
      
      sum=0;
      TRY(PC_replyOK_92p());
      TRY(cable->put(PC_TI92p));
      TRY(cable->put(CMD92p_DATA_PART));
      block_size=4+varsize;
      TRY(cable->put(LSB(block_size)));
      TRY(cable->put(MSB(block_size)));
      for(i=0; i<4; i++)
	{
	  TRY(cable->put(0x00));
	}
      block_size=varsize-2;
      data=MSB(block_size);
      sum+=data;
      TRY(cable->put(data));
      data=LSB(block_size);
      sum+=data;
      TRY(cable->put(data));
      update->total = block_size;
      for(i=0; i<block_size; i++)
	{
	  data=fgetc(file);
	  TRY(cable->put(data));
	  sum+=data;
	  
	  update->count = i;
	  update->percentage = (float)i/block_size;
	  update->pbar();
	  if(update->cancel) return ERR_ABORT;
	}
      fgetc(file); //skips checksum
      fgetc(file);
      TRY(cable->put(LSB(sum)));
      TRY(cable->put(MSB(sum)));
      
      TRY(ti92p_isOK());
      TRY(cable->put(PC_TI92p));
      TRY(cable->put(CMD92p_EOT));
      TRY(cable->put(0x00));
      TRY(cable->put(0x00));
      DISPLAY("The computer does not want continue.\n");
      err = ti92p_isOK();
      if(err)
        {
          DISPLAY("Variable has been rejected by calc.\n");
          sprintf(update->label_text, _("Variable rejected"));
          update->label();
        }
      DISPLAY("\n");
      PAUSE(PAUSE_BETWEEN_VARS);
    }

  update->start();
  TRY(cable->close());

  return 0;
}

int ti92p_recv_backup(FILE *file, int mask_mode, longword *version)
{
  struct varinfo dirlist, *ptr, *ptr2;
  int n;
  int i=0;
  char varname[20];

  TRY(cable->open());
  update->start();

  /* Do a directory list to cable->get variables entries */
  TRY(ti92p_directorylist(&dirlist, &n));

  /* Generate the header of the group file */
  generate_89_92_92p_group_file_header_from_varlist(file, mask_mode, 
						    "**TI92p*", 
						    (&dirlist)->next,
						    CALC_TI92P);

  /* Receive all variables */
  ptr=(&dirlist)->next;
  do
    {
      i++;
      (update->main_percentage)=(float)i/n;
      update->pbar();
      if(update->cancel) return -1;
      DISPLAY("-> %8s %i  %02X %08X\r\n", ptr->varname, (int)(ptr->varattr),
	      ptr->vartype, ptr->varsize);
      if(ptr->vartype == TI92p_DIR)
        ptr->is_folder = FOLDER;
      else
        ptr->is_folder = VARIABLE;
      if(ptr->vartype == TI92p_DIR)
	{
	  ptr=ptr->next;
	  continue;
	}
      if(ptr->vartype == TI92p_FLASH)
        {
          ptr=ptr->next;
          continue;
        }
      TRY(ti92p_isready());
      strcpy(varname, (ptr->folder)->varname);
      strcat(varname, "\\");
      strcat(varname, ptr->varname);
      TRY(ti92p_recv_var(file, mask_mode, 
			    varname, ptr->vartype, ptr->varattr));

      ptr=ptr->next;
    }
  while(ptr != NULL);

  /* Free the allocated memory for the linked list */
  ptr=&dirlist;
  ptr=ptr->next;
  do
    {
      ptr2=ptr->next;
      free(ptr);
      ptr=ptr2;
    }
  while(ptr != NULL);  

  update->start();
  TRY(cable->close());

  return 0;
}

/*
DISPLAY("Sending backup...\r\n");
  sum=0; 
  TRY(cable->put(PC_TI92p));
  TRY(cable->put(CMD92p_VAR_HEADER));
  block_size=4+2+strlen("main");
  TRY(cable->put(LSB(block_size)));
  TRY(cable->put(MSB(block_size)));
  data=0x00;
  for(i=0; i<4; i++)
    {
      TRY(cable->put(data));
    }
  data=TI92p_BKUP;
  sum+=data;
  TRY(cable->put(data));
  data=strlen("main");
  sum+=data;
  TRY(cable->put(data));
  ti92p_sendstring("main", &sum);
  TRY(cable->put(LSB(sum)));
  TRY(cable->put(MSB(sum)));
  TRY(ti92p_isOK());
  TRY(ti92p_waitdata());  
  TRY(PC_replyOK_92p());  
  TRY(cable->put(PC_TI92p));
  TRY(cable->put(CMD92p_EOT));
  TRY(cable->put(0x00));
  TRY(cable->put(0x00));
  DISPLAY("The computer do not want continue.\n");
  TRY(ti92p_isOK());  
*/

int ti92p_send_backup(FILE *file, int mask_mode)
{
  byte data;
  word sum;
  word block_size;
  int i;

  TRY(cable->open());
  update->start();
  DISPLAY("Sending backup...\n");

  /* Send a header */
  sum=0; 
  TRY(cable->put(PC_TI92p));
  TRY(cable->put(CMD92p_VAR_HEADER));
  block_size=4+2+strlen("main");
  TRY(cable->put(LSB(block_size)));
  TRY(cable->put(MSB(block_size)));
  data=0x00;
  for(i=0; i<4; i++)
    {
      TRY(cable->put(data));
    }
  data=TI92p_BKUP;
  sum+=data;
  TRY(cable->put(data));
  data=strlen("main");
  sum+=data;
  TRY(cable->put(data));
  ti92p_sendstring("main", &sum);
  TRY(cable->put(LSB(sum)));
  TRY(cable->put(MSB(sum)));
  TRY(ti92p_isOK());
  TRY(ti92p_waitdata());  
  TRY(PC_replyOK_92p());  
  TRY(cable->put(PC_TI92p));
  TRY(cable->put(CMD92p_EOT));
  TRY(cable->put(0x00));
  TRY(cable->put(0x00));
  DISPLAY("The computer do not want continue.\n");
  TRY(ti92p_isOK());  

  /* Send as a group file with some 'forced' options */
  TRY(ti92p_send_var(file, (mask_mode | MODE_USE_2ND_HEADER |
			    MODE_KEEP_ARCH_ATTRIB) &
		     ~MODE_LOCAL_PATH & ~MODE_DIRLIST));

  update->start();
  TRY(cable->close());

  return 0;
}

#define DUMPROM "dumprom.9xz"

int ti92p_dump_rom(FILE *file, int mask_mode)
{
  int i, j;
  int total;
  byte data;
  time_t start, elapsed, estimated, remaining;
  char buffer[MAXCHARS];
  char tmp[MAXCHARS];
  int pad;
  FILE *f;
  word checksum, sum;
  
  update->start();
  sprintf(update->label_text, "Ready ?");
  update->label();

  /* Open connection and check */
  TRY(cable->open());
  TRY(ti92p_isready());
  TRY(cable->close());
  sprintf(update->label_text, "Yes !");
  update->label();

  /* Transfer ROM dump program from lib to calc */
  f = fopen(DUMPROM, "wb");
  if(f == NULL)
    return -1;
  fwrite(romDump92p, sizeof(unsigned char),
         romDumpSize92p, f);
  fclose(f);
  f = fopen(DUMPROM, "rb");
  TRY(ti92p_send_var(f, MODE_NORMAL));
  fclose(f);
  //unlink(DUMPROM);
  //exit(-1);
  /* Launch calculator program by remote control */
  sprintf(update->label_text, "Launching...");
  update->label();

  TRY(ti92p_send_key(KEY92p_CLEAR));
  PAUSE(50);
  TRY(ti92p_send_key(KEY92p_CLEAR));
  PAUSE(50);
  TRY(ti92p_send_key(KEY92p_m));
  TRY(ti92p_send_key(KEY92p_a));
  TRY(ti92p_send_key(KEY92p_i));
  TRY(ti92p_send_key(KEY92p_n));
  TRY(ti92p_send_key('\\'));
  TRY(ti92p_send_key(KEY92p_d));
  TRY(ti92p_send_key(KEY92p_u));
  TRY(ti92p_send_key(KEY92p_m));
  TRY(ti92p_send_key(KEY92p_p));
  TRY(ti92p_send_key(KEY92p_r));
  TRY(ti92p_send_key(KEY92p_o));
  TRY(ti92p_send_key(KEY92p_m));
  TRY(ti92p_send_key(KEY92p_LP));
  TRY(ti92p_send_key(KEY92p_RP));
  TRY(ti92p_send_key(KEY92p_ENTER));

  /* Receive it now blocks per blocks (1024 + CHK) */
  update->start();
  sprintf(update->label_text, "Receiving...");
  update->label();
  start = time(NULL);
  total = 2 * 1024 * 1024;
  update->total = total;

  for(i=0; i<2*1024; i++)
    {
      sum = 0;
      for (j=0; j<1024; j++)
        {
          TRY(cable->get(&data));
          fprintf(file, "%c", data);
          sum += data;
	  update->percentage = (float)j/1024;
          update->pbar();
          if(update->cancel) return -1;
        }
      TRY(cable->get(&data));
      checksum = data << 8;
      TRY(cable->get(&data));
      checksum |= data;
      if(sum != checksum) return ERR_CHECKSUM;
      TRY(cable->put(0xda));

      update->count = 1024 * 2;
      update->main_percentage = (float)i/(1024*2);
      if(update->cancel) return -1;

      elapsed = difftime(time(NULL), start);
      estimated = elapsed * (float)(1024*2) / i;
      remaining = difftime(estimated, elapsed);
      sprintf(buffer, "%s", ctime(&remaining));
      sscanf(buffer, "%3s %3s %i %s %i", tmp,
             tmp, &pad, tmp, &pad);
      sprintf(update->label_text, "Remaining (mm:ss): %s", tmp+3);
      update->label();
    }

  /* Close connection */
  TRY(cable->close());

  return 0;
}

int ti92p_get_rom_version(char *version)
{
  byte data;
  word sum;
  word checksum;
  int i;
  int b;
  word block_size;
  word num_bytes;

  TRY(cable->open());
  update->start();

  /* Check if TI is ready*/
  TRY(ti92p_isready());  
  
  sum=0;
  num_bytes=0;
  DISPLAY("Request backup...\n");
  /* Request a backup */
  TRY(cable->put(PC_TI92p));
  TRY(cable->put(CMD92p_REQUEST));
  TRY(cable->put(0x12));
  TRY(cable->put(0x00));
  for(i=0; i<4; i++) { TRY(cable->put(0x00)); }
  TRY(cable->put(TI92p_BKUP));
  sum+=0x1D;
  TRY(cable->put(0x0C));
  sum+=0x0C;
  TRY(ti92p_sendstring("main\\version", &sum));
  TRY(cable->put(LSB(sum)));
  TRY(cable->put(MSB(sum)));

  /* Check if TI replies OK */
  TRY(ti92p_isOK());
  
  /* Receive the ROM version */
  sum=0;
  TRY(cable->get(&data));
  if(data != TI92p_PC) return 8;
  TRY(cable->get(&data));
  
  if(data != CMD92p_VAR_HEADER) return 8;
  
  TRY(cable->get(&data));
  if(data == 0x09) { b=0; } else { b=1; }
  TRY(cable->get(&data));
  TRY(cable->get(&data));
  block_size=data;
  sum+=data;
  TRY(cable->get(&data));
  block_size += (data << 8);
  sum+=data;
  for(i=0; i<4; i++)
    {
      TRY(cable->get(&data));
      sum+=data;
    }
  TRY(cable->get(&data));
  version[0]=data;
  sum+=data;
  TRY(cable->get(&data));
  version[1]=data;
  sum+=data;
  TRY(cable->get(&data));
  version[2]=data;
  version[3]='\0';
  sum+=data;
  if(b == 1)
    {
      TRY(cable->get(&data));
      version[3]=data;
      version[4]='\0';
      sum+=data;
    }
  TRY(cable->get(&data));
  checksum=data;
  TRY(cable->get(&data));
  checksum += (data << 8);
  if(checksum != sum) return 9;
  
  /* Abort transfer */  
  TRY(cable->put(PC_TI92p));
  TRY(cable->put(CMD92p_CHK_ERROR));
  TRY(cable->put(0x00));
  TRY(cable->put(0x00));
  TRY(cable->put(PC_TI92p));
  TRY(cable->put(CMD92p_CHK_ERROR));
  TRY(cable->put(0x00));
  TRY(cable->put(0x00));
  DISPLAY("ROM version %s\n", version);
  
  DISPLAY("\n");
  update->start();
  TRY(cable->close());	
  
  return 0;
}

int ti92p_get_idlist(char *id)
{
  byte data;
  word sum;
  word checksum;
  word block_size;
  int i, j;
  longword var_size;
  byte name_length;
  char name[9];

  TRY(cable->open());
  update->start();
  sum=0;
  DISPLAY("Request IDlist...\n");
  TRY(cable->put(PC_TI92p));
  TRY(cable->put(CMD92p_REQUEST));
  block_size=4+2;
  data=LSB(block_size);
  TRY(cable->put(data));
  data=MSB(block_size);
  TRY(cable->put(data));
  for(i=0; i<4; i++)
    {
      TRY(cable->put(0x00));
    }
  data=TI92p_IDLIST;
  TRY(cable->put(data));
  sum+=data;
  data=0x00;
  TRY(cable->put(data));
  sum+=data;
  TRY(cable->put(LSB(sum)));
  TRY(cable->put(MSB(sum)));
  
  TRY(ti92p_isOK());
  sum=0;
  TRY(cable->get(&data));
  if(data != TI92p_PC) return ERR_INVALID_BYTE;
  TRY(cable->get(&data));
  if(data != CMD92p_VAR_HEADER) return ERR_INVALID_BYTE;
  TRY(cable->get(&data));
  TRY(cable->get(&data));
  TRY(cable->get(&data));
  var_size=data;
  sum+=data;
  TRY(cable->get(&data));
  var_size |= (data << 8);
  sum+=data;
  TRY(cable->get(&data));
  var_size |= (data << 16);
  sum+=data;
  TRY(cable->get(&data));
  var_size |= (data << 24);
  sum+=data;
  DISPLAY("Size of the IDlist: 0x%08X = %i.\n", var_size-2, var_size-2);
  TRY(cable->get(&data));
  DISPLAY("Type of the variable: %s\n", ti92p_byte2type(data));
  sum+=data;
  TRY(cable->get(&data));
  name_length=data;
  sum+=data;
  DISPLAY("Variable name: ");
  for(i=0; i<name_length; i++)
    {
      TRY(cable->get(&data));
      name[i]=data;
      DISPLAY("%c", data);
      sum+=data;
    }
  name[i]='\0';
  DISPLAY("\n");
  TRY(cable->get(&data)); // It's the only difference with the 89
  sum+=data;
  TRY(cable->get(&data));
  checksum=data;
  TRY(cable->get(&data));
  checksum += (data << 8);
  if(checksum != sum) return ERR_CHECKSUM;

  TRY(PC_replyOK_92p());
  TRY(PC_waitdata_92p());
  DISPLAY("The calculator want continue.\n");

  TRY(ti92p_isOK());
  DISPLAY("Receiving IDlist...\n");
  TRY(cable->get(&data));
  if(data != TI92p_PC) return ERR_INVALID_BYTE;
  TRY(cable->get(&data));
  if(data != CMD92p_DATA_PART) return ERR_INVALID_BYTE;
  TRY(cable->get(&data));
  TRY(cable->get(&data));
  sum=0;
  for(i=0; i<4; i++)
    {
      TRY(cable->get(&data));
      sum+=data;
    }
  TRY(cable->get(&data));
  block_size = (data << 8);
  sum+=data;
  TRY(cable->get(&data));
  block_size |= data;
  sum+=data;
  block_size=var_size-2;
  for(i=0, j=0; i<block_size; i++, j++)
    {
      TRY(cable->get(&data));
      sum+=data;
      if( (j == 7) || (j == 13) )
	{
	  id[j]='-';
	  j++;
	}
      id[j]=data;
    }
  id[j]='\0';
  DISPLAY("IDlist: <%s>\n", id+2);
  TRY(cable->get(&data));
  checksum=data;
  TRY(cable->get(&data));
  checksum += (data << 8);
  if(checksum != sum) return ERR_CHECKSUM;
  TRY(PC_replyOK_92p());
  TRY(cable->get(&data));
  if(data != TI92p_PC) return ERR_CHECKSUM;
  TRY(cable->get(&data));
  DISPLAY("The calculator do not want continue.\n");
  TRY(cable->get(&data));
  TRY(cable->get(&data));

  TRY(cable->put(PC_TI92p));
  TRY(cable->put(CMD92p_TI_OK));
  TRY(cable->put(0x00));
  TRY(cable->put(0x00));
  DISPLAY("\n");
  update->start();
  TRY(cable->close());

  return 0;
}

int ti92p_send_flash(FILE *file, int mask_mode)
{
  byte data;
  word sum;
  char flash_name[128];
  longword flash_size=0;
  longword block_size;
  int i, j;
  int num_blocks;
  word last_block;
  char *signature = "Advanced Mathematics Software";
  int tib = 0;
  Ti92pFlashHeader header;
  long file_size, offset, pos=0;

  //LOCK_TRANSFER();
  /* Read the file header and initialize some variables */
  TRY(cable->open());
  update_start();
  fgets(flash_name, 128, file);
  if(strstr(flash_name, "**TIFL**") == NULL) // is a .89u file
    {
      for(i=0, j=0; i<127; i++) // is a .tib file
	{
	  if(flash_name[i] == signature[j])
	    {
	      j++;
	      if(j==strlen(signature))
		{
		  DISPLAY("TIB file.\n");
		  tib = 1;
		  break;
		}
	    }
	}
      if(j < strlen(signature))
	return ERR_INVALID_FLASH_FILE; // not a FLASH file
    }

  rewind(file);
  
  if(tib)
    { // tib is an old format
      fseek(file, 0, SEEK_END);
      flash_size = ftell(file);
      fseek(file, 0, SEEK_SET);
      strcpy(flash_name, "basecode");
    }
  else
    {
      fseek(file, 0, SEEK_END);
      file_size = ftell(file);
      fseek(file, 0, SEEK_SET);
      
      for(i=0, offset=2*sizeof(Ti92pFlashHeader); offset<file_size; i++)
	{
	  fread(&header, sizeof(Ti92pFlashHeader), 1, file);
	  flash_size = header.data_length[0] + (header.data_length[1] << 8) +
	    (header.data_length[2] << 16) + (header.data_length[3] << 24);
	  
	  if(memcmp(header.id, "**TIFL**", 8))
	    return ERR_INVALID_FLASH_FILE;

	  DISPLAY("Header #%i\n", i);
	  DISPLAY(" id = <%s>\n", header.id);
	  DISPLAY(" revision = %i.%i\n", LSB(header.revision), 
		  MSB(header.revision));
	  DISPLAY(" flags = %i\n", header.flags);
	  DISPLAY(" type = %i\n", header.object_type);
	  DISPLAY(" date : %02X/%02X/%02X%02X\n", 
		  header.revision_day, header.revision_month, 
		  LSB(header.revision_year), MSB(header.revision_year));
	  DISPLAY(" name = <%s>\n", header.name);
	  DISPLAY(" device_type = %02X\n", header.device_type);
	  DISPLAY(" data_type = %02X\n", header.data_type);
	  DISPLAY(" size = %i\n", flash_size);
	  DISPLAY("\n");

	  pos = ftell(file);
	  fseek(file, flash_size, SEEK_CUR);
	  offset += flash_size;
	}

      fseek(file, pos, SEEK_SET);
      strcpy(flash_name, header.name);
    }

  
  DISPLAY("\n");
  DISPLAY("Sending FLASH app/os...\n");
  DISPLAY("FLASH app/os name: \"%s\"\n", flash_name);
  DISPLAY("FLASH app/os size: %i bytes.\n", flash_size);

  /* Now, read data from the file and send them by block */
  sum=0;
  TRY(cable->put(PC_TI92p));
  TRY(cable->put(CMD92p_VAR_HEADER2));
  if(mask_mode == MODE_AMS)
    block_size = 4+2;
  else
    block_size=4+2+strlen(flash_name);
  TRY(cable->put(LSB(block_size)));
  TRY(cable->put(MSB(block_size)));
  data=flash_size & 0x000000FF;
  sum+=data;
  TRY(cable->put(data));
  data=(flash_size & 0x0000FF00) >> 8;
  sum+=data;
  TRY(cable->put(data));
  data=(flash_size & 0x00FF0000) >> 16;
  sum+=data;
  TRY(cable->put(data));
  data=(flash_size & 0xFF000000) >> 24;
  sum+=data;
  TRY(cable->put(data));
  if(mask_mode == MODE_AMS)
    data = TI92p_AMS;
  else
    data = TI92p_FLASH;
  sum+=data;
  TRY(cable->put(data));
  if(mask_mode == MODE_AMS)
    data=0x00;
  else
    data=strlen(flash_name);
  sum+=data;
  TRY(cable->put(data));
  if(mask_mode != MODE_AMS)
    ti92p_sendstring(flash_name, &sum);
  TRY(cable->put(LSB(sum)));
  TRY(cable->put(MSB(sum)));

  num_blocks = flash_size/65536;
  DISPLAY("Number of blocks: %i\n", num_blocks + 1);
  
  for(i=0; i<num_blocks; i++ )
    {
      DISPLAY("Sending block %i.\n", i);
      TRY(ti92p_isOK());
      TRY(ti92p_waitdata());
      TRY(PC_replyOK_92p());

      sum=0;
      TRY(cable->put(PC_TI92p));
      TRY(cable->put(CMD92p_DATA_PART));
      block_size=65536;
      TRY(cable->put(LSB(block_size)));
      TRY(cable->put(MSB(block_size)));
      DISPLAY("Transmitting data.\n");
      update->total = 65536;
      for(j=0; j<65536; j++)
 	{
	  data=fgetc(file);
	  sum+=data;
	  TRY(cable->put(data));

	  update->count = j;
	  update->percentage = (float)j/65536;
	  update_pbar();
	  if(update->cancel) return ERR_ABORT;
	}
      TRY(cable->put(LSB(sum)));
      TRY(cable->put(MSB(sum)));

      TRY(ti92p_isOK());
      TRY(PC_replyCONT_92p());
      
      ((update->main_percentage))=(float)i/num_blocks;
      update_pbar();
      if(update->cancel) return ERR_ABORT;
    }

  DISPLAY("Sending the last block.\n");
  last_block=flash_size % 65536;
  sum=0;
  TRY(ti92p_isOK());
  TRY(ti92p_waitdata());
  TRY(PC_replyOK_92p());
  
  sum=0;
  TRY(cable->put(PC_TI92p));
  TRY(cable->put(CMD92p_DATA_PART));
  block_size=last_block;
  TRY(cable->put(LSB(block_size)));
  TRY(cable->put(MSB(block_size)));
  DISPLAY("Transmitting data.\n");
  update->total = last_block;
  for(j=0; j<last_block; j++)
    {
      data=fgetc(file);
      sum+=data;
      TRY(cable->put(data));

      update->count = j;
      update->percentage = (float)j/last_block;
      update_pbar();
      if(update->cancel) return ERR_ABORT;
    }
  TRY(cable->put(LSB(sum)));
  TRY(cable->put(MSB(sum)));
  
  TRY(ti92p_isOK());
  
  TRY(cable->put(PC_TI92p));
  TRY(cable->put(CMD92p_EOT));
  TRY(cable->put(0x00));
  TRY(cable->put(0x00));

  if(mask_mode != MODE_APPS)
    DISPLAY("Flash application sent completely.\n");
  if(mask_mode == MODE_AMS)
    {
      TRY(ti92p_isOK());
      DISPLAY("Operating System sent completely.\n");
    }
  DISPLAY("\n");

  update_start();
  TRY(cable->close());
  //UNLOCK_TRANSFER();

  return 0;
}

int ti92p_recv_flash(FILE *file, int mask_mode, char *appname)
{
  byte data;
  word sum;
  word checksum;
  word block_size;
  int i,j;
  longword var_size;
  byte name_length;
  char name[9];
  char *varname = appname;

  fprintf(file, "**TIFL**");
  for(i=0; i<4; i++)
    fputc(0x00, file);
  for(i=0; i<4; i++) //date: 24 01 2000
    fputc(0x00, file);
  fputc(strlen(appname), file);
  fprintf(file, "%s", appname);
  for(i=16+strlen(appname)+1; i<0x4A; i++)
    fputc(0x00, file);
  TRY(cable->open());
  update->start();
  sprintf(update->label_text, "Application: %s", varname);
  update->label();
  sum=0;
  DISPLAY("Request FLASH application: %s\n", varname);
  TRY(cable->put(PC_TI92p));
  TRY(cable->put(CMD92p_REQUEST));
  block_size=4+2+strlen(varname);
  data=LSB(block_size);
  TRY(cable->put(data));
  data=MSB(block_size);
  TRY(cable->put(data));
  for(i=0; i<4; i++)
    {
      TRY(cable->put(0x00));
    }
  data=TI92p_FLASH;
  TRY(cable->put(data));
  sum+=data;
  data=strlen(varname);
  TRY(cable->put(data));
  sum+=data;
  TRY(ti92p_sendstring(varname, &sum));
  TRY(cable->put(LSB(sum)));
  TRY(cable->put(MSB(sum)));
  
  TRY(ti92p_isOK());
  sum=0;
  TRY(cable->get(&data));
  if(data != TI92p_PC) return ERR_INVALID_BYTE;
  TRY(cable->get(&data));
  if(data != CMD92p_VAR_HEADER) return ERR_PACKET;
  TRY(cable->get(&data));
  TRY(cable->get(&data));
  TRY(cable->get(&data));
  var_size=data;
  sum+=data;
  TRY(cable->get(&data));
  var_size |= (data << 8);
  sum+=data;
  TRY(cable->get(&data));
  var_size |= (data << 16);
  sum+=data;
  TRY(cable->get(&data));
  var_size |= (data << 24);
  sum+=data;
  DISPLAY("Size of the app in memory: 0x%08X = %i.\n", var_size, var_size);
  fputc(LSB(LSW(var_size)), file);
  fputc(MSB(LSW(var_size)), file);
  fputc(LSB(MSW(var_size)), file);
  fputc(MSB(MSW(var_size)), file);
  TRY(cable->get(&data));
  DISPLAY("Type of the application: %s\n", ti92p_byte2type(data));
  sum+=data;
  TRY(cable->get(&data));
  name_length=data;
  sum+=data;
  DISPLAY("Application name: ");
  for(i=0; i<name_length; i++)
    {
      TRY(cable->get(&data));
      name[i]=data;
      DISPLAY("%c", data);
      sum+=data;
    }
  name[i]='\0';
  DISPLAY("\n");
  TRY(cable->get(&data)); // It's the only difference with the 92
  sum+=data;
  TRY(cable->get(&data));
  checksum=data;
  TRY(cable->get(&data));
  checksum += (data << 8);
  if(checksum != sum) return ERR_CHECKSUM;

  DISPLAY("Receiving application...\n");
  for(i=0, block_size=0; i<var_size; i+=block_size)
    {
      TRY(PC_replyOK_92p());
      TRY(PC_waitdata_92p());
      DISPLAY("The calculator want continue.\n");
      
      TRY(ti92p_isOK());
      DISPLAY("Receiving block");
      TRY(cable->get(&data));
      if(data != TI92p_PC) return ERR_INVALID_BYTE;
      TRY(cable->get(&data));
      if(data != CMD92p_DATA_PART) return ERR_INVALID_BYTE;
      TRY(cable->get(&data));
      block_size = data;
      TRY(cable->get(&data));
      block_size |= (data << 8);
      update->total = block_size;
      DISPLAY(", size = 0x%04x...\n", block_size);
      sum=0;
      for(j=0; j<block_size; j++)
	{
	  TRY(cable->get(&data));
	  sum+=data;
	  fprintf(file, "%c", data);
	  
	  update->count = j;
	  update->percentage = (float)j/block_size;
	  update->pbar();
	  if(update->cancel) return ERR_ABORT;
	}
      TRY(cable->get(&data));
      checksum=data;
      TRY(cable->get(&data));
      checksum += (data << 8);
      if(checksum != sum) return ERR_CHECKSUM;
      
      TRY(PC_replyOK_92p());
      TRY(cable->get(&data));
      if(data != TI92p_PC) return ERR_INVALID_BYTE;
      TRY(cable->get(&data));
      if(data == CMD92p_CONTINUE)
	DISPLAY("The calculator wants to continue.\n");
      else if(data == CMD92p_EOT)
	DISPLAY("The calculator does not want to continue.\n");
      else
	return ERR_INVALID_BYTE;
      TRY(cable->get(&data));
      TRY(cable->get(&data));
      
      ((update->main_percentage))=(float)i/var_size;
      update_pbar();
      if(update->cancel) return ERR_ABORT;
    }
      TRY(PC_replyOK_92p());
      DISPLAY("\n");

  update->start();
  TRY(cable->close());
  PAUSE(PAUSE_BETWEEN_VARS);

  return 0;
}

int ti92p_supported_operations(void)
{
   return 
    (
     OPS_ISREADY |
     OPS_SCREENDUMP |
     OPS_SEND_KEY | OPS_RECV_KEY | OPS_REMOTE |
     OPS_DIRLIST |
     OPS_SEND_BACKUP | OPS_RECV_BACKUP |
     OPS_SEND_VARS | OPS_RECV_VARS |
     OPS_SEND_FLASH | OPS_RECV_FLASH |
     OPS_IDLIST |
     OPS_ROMDUMP
     );
}

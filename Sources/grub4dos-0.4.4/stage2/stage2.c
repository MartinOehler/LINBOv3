/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2000,2001,2002,2004,2005  Free Software Foundation, Inc.
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <shared.h>
#include <term.h>

#ifdef SUPPORT_GRAPHICS
extern int disable_space_highlight;
#endif /* SUPPORT_GRAPHICS */

#ifdef GRUB_UTIL

grub_jmp_buf restart_env;

/* The preset_menu variable is referenced by asm.S */
# if defined(PRESET_MENU_STRING)
const char *preset_menu = PRESET_MENU_STRING;
# elif defined(SUPPORT_DISKLESS)
/* Execute the command "bootp" automatically.  */
const char *preset_menu = "bootp\n";
#else /* ! SUPPORT_DISKLESS */
/* The preset_menu variable is always available. This is for the loader, which
 * loads GRUB, to define a new preset menu before transferring control to GRUB.
 */
const char *preset_menu = 0;
# endif /* ! SUPPORT_DISKLESS */
#else /* ! GRUB_UTIL */
/* preset_menu is defined in asm.S */
#endif /* GRUB_UTIL */

static unsigned long preset_menu_offset;

static int
open_preset_menu (void)
{
#ifdef GRUB_UTIL
  /* Unless the user explicitly requests to use the preset menu,
     always opening the preset menu fails in the grub shell.  */
  if (! use_preset_menu)
    return 0;
#endif /* GRUB_UTIL */
  
  preset_menu_offset = 0;
  return preset_menu != 0;
}

static int
read_from_preset_menu (char *buf, int max_len)
{
  int len;

  if (preset_menu == 0)
	return 0;

  len = grub_strlen (preset_menu + preset_menu_offset);

  if (len > max_len)
    len = max_len;

  grub_memmove (buf, preset_menu + preset_menu_offset, len);
  preset_menu_offset += len;

  return len;
}

#ifdef GRUB_UTIL
#undef	DISP_UL		//218
#undef	DISP_UR		//191
#undef	DISP_LL		//192
#undef	DISP_LR		//217
#undef	DISP_HORIZ	//196
#undef	DISP_VERT	//179
#undef	DISP_LEFT	//0x1b
#undef	DISP_RIGHT	//0x1a
#undef	DISP_UP		//0x18
#undef	DISP_DOWN	//0x19

#define DISP_UL		ACS_ULCORNER	/* 0xDA , '+' */
#define DISP_UR		ACS_URCORNER	/* 0xBF , '+' */
#define DISP_LL		ACS_LLCORNER	/* 0xC0 , '+' */
#define DISP_LR		ACS_LRCORNER	/* 0xD9 , '+' */
#define DISP_HORIZ	ACS_HLINE	/* 0xC4 , '-' */
#define DISP_VERT	ACS_VLINE	/* 0xB3 , '|' */
#define DISP_LEFT	ACS_LARROW	/* 0x1B , '<' */
#define	DISP_RIGHT	ACS_RARROW	/* 0x1A , '>' */
#define DISP_UP		ACS_UARROW	/* 0x18 , '^' */
#define DISP_DOWN	ACS_DARROW	/* 0x19 , 'v' */
#endif

/* line start */
#define MENU_BOX_X	2

/* line width */
#define MENU_BOX_W	76

/* first line number */
#define MENU_BOX_Y	2

/* window height */
#define MENU_BOX_H	(current_term->max_lines - 8)

/* line end */
#define MENU_BOX_E	(MENU_BOX_X + MENU_BOX_W)

/* window bottom */
#define MENU_BOX_B	(MENU_BOX_Y + MENU_BOX_H)

static long temp_entryno;
static char * *titles;	/* title array, point to 256 strings. */
static int default_help_message_destoyed = 1;

static void
print_default_help_message (char *config_entries)
{
      grub_printf ("\n Use the %c and %c keys to highlight an entry.",
		   DISP_UP, DISP_DOWN);
      
      if (! auth && password)
	{
#if 0
#ifdef SUPPORT_GFX
	  if (*graphics_file)
	    {
	      grub_putstr ("\
	WARNING: graphical menu doesn\'t work\
	in conjunction with the password feature\n" );
	    }
#endif
#endif
	  grub_putstr (" Press ENTER or \'b\' to boot.\n"
		" Press \'p\' to gain privileged control.");
	}
      else
	{
	  if (config_entries)
	    grub_putstr (" Press ENTER or \'b\' to boot.\n"
		    " Press \'e\' to edit the commands before booting, or \'c\' for a command-line.");
	  else
	    grub_putstr (" At a selected line, press \'e\' to\n"
		" edit, \'d\' to delete, or \'O\'/\'o\' to open a new line before/after. When done,\n"
		" press \'b\' to boot, \'c\' for a command-line, or ESC to go back to the main menu.");
	}

      default_help_message_destoyed = 0;
}

static char *
get_entry (char *list, int num)
{
  int i;

  if (list == (char *)titles)
	return titles[num];

  for (i = 0; i < num; i++)
    {
	while (*(list++));
    }

  return list;
}

/* Print an entry in a line of the menu box.  */
static void
print_entry (int y, int highlight, char *entry, char *config_entries)
{
  int x;
  unsigned char c = (entry ? (unsigned char)*entry : 0);

  if (current_term->setcolorstate)
    current_term->setcolorstate (highlight ? COLOR_STATE_HIGHLIGHT : COLOR_STATE_NORMAL);
  
//  if (highlight && current_term->setcolorstate)
//    current_term->setcolorstate (COLOR_STATE_HIGHLIGHT);

  gotoxy (MENU_BOX_X - 1, y);
#ifdef SUPPORT_GRAPHICS
  disable_space_highlight = 1;
#endif /* SUPPORT_GRAPHICS */
  grub_putchar (' ');
#ifdef SUPPORT_GRAPHICS
  disable_space_highlight = 0;
#endif /* SUPPORT_GRAPHICS */
  for (x = MENU_BOX_X; x < MENU_BOX_E; x++)
    {
      if (c && c != '\n' && x <= MENU_BOX_W)
	{
	  if (x == MENU_BOX_W)
	    grub_putchar (DISP_RIGHT);
	  else
	  {
	    while (c == 8 || c == 9)//(c && (c <= 0x0F))
	      c = *(++entry);
	    if (! c || c == '\n')
		goto space_no_highlight;
	    grub_putchar (c);
	    c = *(++entry);
	  }
	}
      else
      {
space_no_highlight:
#ifdef SUPPORT_GRAPHICS
	if (! disable_space_highlight)
	      disable_space_highlight = 1;
#endif /* SUPPORT_GRAPHICS */
	grub_putchar (' ');
      }
    }
#ifdef SUPPORT_GRAPHICS
  disable_space_highlight = 0;
#endif /* SUPPORT_GRAPHICS */
  //gotoxy (MENU_BOX_E - 1, y);		/* XXX: Why? */

  if (highlight && config_entries)
  {
	int j;

	if (current_term->setcolorstate)
	    current_term->setcolorstate (COLOR_STATE_HELPTEXT);

	if (entry && (c = *entry) == '\n')
	{
		default_help_message_destoyed = 1;
		//c = *(++entry);
		for (j = MENU_BOX_B + 1; j < MENU_BOX_B + 5; j++)
		{
			if (c == '\n')
				c = *(++entry);
			gotoxy (MENU_BOX_X - 2, j);
			for (x = 0; x < 79; x++)
			{
				if (c && c != '\n')
				{
					if (c == '\r')
						x = 0;
					grub_putchar (c);
					c = *(++entry);
				}
				else
					grub_putchar (' ');
			}
		}
		//gotoxy (MENU_BOX_X - 2, MENU_BOX_B + 1);
		//grub_putstr (++entry);
		gotoxy (MENU_BOX_E, y);
	}
	else if (default_help_message_destoyed)
	{
		for (j = MENU_BOX_B + 1; j < MENU_BOX_B + 6; j++)
		{
			gotoxy (MENU_BOX_X - 2, j);
			for (x = 0; x < 79; x++)
				grub_putchar (' ');
		}
		gotoxy (MENU_BOX_X - 2, MENU_BOX_B + 1);
		print_default_help_message (config_entries);
		gotoxy (MENU_BOX_E, y);
	}
  }

  if (current_term->setcolorstate)
    current_term->setcolorstate (COLOR_STATE_STANDARD);
}

/* Print entries in the menu box.  */
static void
print_entries (int first, int entryno, char *menu_entries, char *config_entries)
{
  int i;
  int main_menu = (menu_entries == (char *)titles);
  
  if (current_term->setcolorstate)
    current_term->setcolorstate (COLOR_STATE_NORMAL);
  
  gotoxy (MENU_BOX_E, MENU_BOX_Y);

  if (first)
    grub_putchar (DISP_UP);
  else
    grub_putchar (DISP_VERT);

  if (main_menu || *menu_entries)
    menu_entries = get_entry (menu_entries, first);

  for (i = 0; i < MENU_BOX_H/*size*/; i++)
    {
      print_entry (MENU_BOX_Y + i, entryno == i, menu_entries, config_entries);

      if (main_menu)
      {
	if (menu_entries)
	{
	    if ((++first) < 256)
		menu_entries = titles[first];
	    else
		menu_entries = 0;
	}
	continue;
      }
      if (*menu_entries)
	while (*(menu_entries++));
    }

  gotoxy (MENU_BOX_E, MENU_BOX_Y - 1 + MENU_BOX_H/*size*/);

  if (current_term->setcolorstate)
    current_term->setcolorstate (COLOR_STATE_NORMAL);
  
  if (menu_entries && *menu_entries)
    grub_putchar (DISP_DOWN);
  else
    grub_putchar (DISP_VERT);

  if (current_term->setcolorstate)
    current_term->setcolorstate (COLOR_STATE_STANDARD);

  gotoxy (MENU_BOX_E, MENU_BOX_Y + entryno);	/* XXX: Why? */
}

static void
print_entries_raw (int size, int first, char *menu_entries)
{
  int i;
  char *p;

  for (i = 0; i < MENU_BOX_W; i++)
    grub_putchar ('-');
  grub_putchar ('\n');

  for (i = first; i < size; i++)
    {
      p = get_entry (menu_entries, i);
      grub_printf ("%02d: %s\n", i, (((*p) & 0xF0) ? p : ++p));
    }

  for (i = 0; i < MENU_BOX_W; i++)
    grub_putchar ('-');
  grub_putchar ('\n');
  grub_putchar ('\n');
}


static void
print_border (int y, int size)
{
  int i;

  if (current_term->setcolorstate)
    current_term->setcolorstate (COLOR_STATE_NORMAL);
  
  gotoxy (MENU_BOX_X - 2, y);

  grub_putchar (DISP_UL);
  for (i = 0; i < MENU_BOX_W + 1; i++)
    grub_putchar (DISP_HORIZ);
  grub_putchar (DISP_UR);

  i = 1;
  while (1)
    {
      gotoxy (MENU_BOX_X - 2, y + i);

      if (i > size)
	break;
      
      grub_putchar (DISP_VERT);
      gotoxy (MENU_BOX_E, y + i);
      grub_putchar (DISP_VERT);

      i++;
    }

  grub_putchar (DISP_LL);
  for (i = 0; i < MENU_BOX_W + 1; i++)
    grub_putchar (DISP_HORIZ);
  grub_putchar (DISP_LR);

  gotoxy (MENU_BOX_X - 2, MENU_BOX_B + 1);

  if (current_term->setcolorstate)
    current_term->setcolorstate (COLOR_STATE_STANDARD);
}

extern int commandline_func (char *arg1, int flags);
extern int errnum_func (char *arg1, int flags);
extern int checkrange_func (char *arg1, int flags);

/* Run an entry from the script SCRIPT. HEAP is used for the
   command-line buffer. If an error occurs, return non-zero, otherwise
   return zero.  */
static int
run_script (char *script, char *heap)
{
  char *old_entry = 0;
  char *cur_entry = script;
  struct builtin *builtin = 0;
  char *arg;
  grub_error_t errnum_old;

  /* Initialize the data.  */
  //saved_drive = boot_drive;
  //saved_partition = install_partition;
  current_drive = GRUB_INVALID_DRIVE;
  count_lines = -1;
  
  /* Initialize the data for the builtin commands.  */
  kernel_type = KERNEL_TYPE_NONE;
  errnum = 0;
  errorcheck = 1;	/* errorcheck on */

  while (1)
    {
      if (errnum && errorcheck)
	break;

      errnum_old = errnum;
      /* Copy the first string in CUR_ENTRY to HEAP.  */
      errnum = ERR_NONE;
      old_entry = cur_entry;
      while (*cur_entry++)
	;

      grub_memmove (heap, old_entry, (int) cur_entry - (int) old_entry);
      if (! *heap)
	{
	  /* If there is no more command in SCRIPT...  */

	  /* If any kernel is not loaded, just exit successfully.  */
	  if (kernel_type == KERNEL_TYPE_NONE)
	    return 0;	/* return to main menu. */

	  /* Otherwise, the command boot is run implicitly.  */
	  grub_memmove (heap, "boot", 5);
	}

      /* Find a builtin.  */
      builtin = find_command (heap);
      if (! builtin)
	{
	  grub_printf ("%s\n", old_entry);
	  continue;
	}
/* now command echoing is considered no use for a successful command. */
//      if (! (builtin->flags & BUILTIN_NO_ECHO))
//	grub_printf ("%s\n", old_entry);

      /* If BUILTIN cannot be run in the command-line, skip it.  */
      if (! (builtin->flags & BUILTIN_CMDLINE))
	{
	  errnum = ERR_UNRECOGNIZED;
	  continue;
	}

      /* Invalidate the cache, because the user may exchange removable
	 disks.  */
      buf_drive = -1;

      if ((builtin->func) == errnum_func || (builtin->func) == checkrange_func)
	errnum = errnum_old;

      /* find && and || */

      for (arg = skip_to (0, heap); *arg != 0; arg = skip_to (0, arg))
      {
	struct builtin *builtin1;
	int ret;
	char *arg1;
	arg1 = arg;
        if (*arg == '&' && arg[1] == '&' && (arg[2] == ' ' || arg[2] == '\t'))
        {
		/* handle the AND operator */
		arg = skip_to (0, arg);
		builtin1 = find_command (arg);
		if (! builtin1 || ! (builtin1->flags & BUILTIN_CMDLINE))
		{
			errnum = ERR_UNRECOGNIZED;
			goto next;
		}

		*arg1 = 0;
		ret = (builtin->func) (skip_to (1, heap), BUILTIN_SCRIPT);
		*arg1 = '&';
		if (ret)
		{
			arg = skip_to (1, arg);
			if ((builtin1->func) != errnum_func && (builtin1->func) != checkrange_func)
				errnum = 0;
			(builtin1->func) (arg, BUILTIN_SCRIPT);
		} else
			errnum = 0;
		goto next;
	} else if (*arg == '|' && arg[1] == '|' && (arg[2] == ' ' || arg[2] == '\t'))
	{
		/* handle the OR operator */
		arg = skip_to (0, arg);
		builtin1 = find_command (arg);
		if (! builtin1 || ! (builtin1->flags & BUILTIN_CMDLINE))
		{
			errnum = ERR_UNRECOGNIZED;
			goto next;
		}

		*arg1 = 0;
		ret = (builtin->func) (skip_to (1, heap), BUILTIN_SCRIPT);
		*arg1 = '|';
		if (! ret)
		{
			arg = skip_to (1, arg);
			if ((builtin1->func) != errnum_func && (builtin1->func) != checkrange_func)
				errnum = 0;
			(builtin1->func) (arg, BUILTIN_SCRIPT);
		} else
			errnum = 0;
		goto next;
	}
      }

	/* Run BUILTIN->FUNC.  */
	arg = (builtin->func) == commandline_func ? heap : skip_to (1, heap);
	(builtin->func) (arg, BUILTIN_SCRIPT);
next:
      if (! *old_entry)	/* HEAP holds the implicit BOOT command */
	break;
    } /* while (1) */

  kernel_type = KERNEL_TYPE_NONE;

  /* If a fallback entry is defined, don't prompt a user's intervention.  */
  
  if (fallback_entryno < 0)
    {
      if (! (builtin->flags & BUILTIN_NO_ECHO))
	grub_printf ("%s\n", heap);
      print_error ();

      grub_printf ("\nPress any key to continue...");
      (void) getkey ();
    }
	  
  errnum = ERR_NONE;
  return 1;	/* use fallback. */
}

static int fallbacked_entries;
static int old_c;
static int old_c_count;
static int old_c_count_end;

static void
run_menu (char *menu_entries, char *config_entries, int num_entries, char *heap, int entryno)
{
  int c, time1, time2 = -1, first_entry = 0;
  char *cur_entry = 0;
//  struct term_entry *prev_term = NULL;
		  

  /*
   *  Main loop for menu UI.
   */

restart:
  /* Dumb terminal always use all entries for display 
     invariant for TERM_DUMB: first_entry == 0  */
  if (! (current_term->flags & TERM_DUMB))
    {
      if (entryno > MENU_BOX_H - 1)
	{
	  first_entry += entryno - (MENU_BOX_H - 1);
	  entryno = MENU_BOX_H - 1;
	}
    }

  /* If the timeout was expired or wasn't set, force to show the menu
     interface. */
  if (grub_timeout < 0)
    show_menu = 1;
  
  /* If SHOW_MENU is false, don't display the menu until a key-press.  */
  if (! show_menu)
    {
      cls();	//clear screen so splash image will work

      /* Get current time.  */
      while ((time1 = getrtsecs ()) == 0xFF)
	;

      while (1)
	{
	  /* Unhide the menu on any keypress.  */
	  if (checkkey () != -1 /*&& ASCII_CHAR (getkey ()) == '\e'*/)
	    {
	      getkey ();	/* eat the key */
	      grub_timeout = -1;
	      show_menu = 1;
	      break;
	    }

	  /* If GRUB_TIMEOUT is expired, boot the default entry.  */
	  if (grub_timeout >=0
	      && (time1 = getrtsecs ()) != time2
	      && time1 != 0xFF)
	    {
	      if (grub_timeout <= 0)
		{
		  grub_timeout = -1;
		  goto boot_entry;
		}
	      
	      time2 = time1;
	      grub_timeout--;
	      
	      /* Print a message.  */
	      if (! silent_hiddenmenu)
	      {
		grub_printf ("\rPress any key to enter the menu... %d   ",
				grub_timeout);
	      }
	    }
	}
    }

  /* Only display the menu if the user wants to see it. */
  if (show_menu)
    {
      init_page ();
      setcursor (0);

      if (current_term->flags & TERM_DUMB)
	print_entries_raw (num_entries, first_entry, menu_entries);
      else
	print_border (MENU_BOX_Y - 1, MENU_BOX_H);

      if (current_term->setcolorstate)
	  current_term->setcolorstate (COLOR_STATE_HELPTEXT);

	{
		int j;
		int x;

		for (j = MENU_BOX_B + 1; j < MENU_BOX_B + 6; j++)
		{
			gotoxy (MENU_BOX_X - 2, j);
			for (x = 0; x < 79; x++)
				grub_putchar (' ');
		}
		gotoxy (MENU_BOX_X - 2, MENU_BOX_B + 1);
	}

      print_default_help_message (config_entries);

      if (current_term->flags & TERM_DUMB)
	grub_printf ("\n\nThe selected entry is %d ", entryno);
      else
	print_entries (first_entry, entryno, menu_entries, config_entries);
    }

  /* XX using RT clock now, need to initialize value */
  while ((time1 = getrtsecs()) == 0xFF);

  old_c = 0;
  old_c_count = 0;
  old_c_count_end = 0;
  temp_entryno = 0;
  /* Initialize to NULL just in case...  */
  cur_entry = NULL;
  while (1)
    {
      /* Initialize to NULL just in case...  */
      //cur_entry = NULL;
	//cur_entry = menu_entries; /* for modified menu */

      if (grub_timeout >= 0 && (time1 = getrtsecs()) != time2 && time1 != 0xFF)
	{
	  if (grub_timeout <= 0)
	    {
	      grub_timeout = -1;
	      break;
	    }

	  /* else not booting yet! */
	  time2 = time1;

	  if (current_term->setcolorstate)
	      current_term->setcolorstate (COLOR_STATE_HELPTEXT);

	  if (current_term->flags & TERM_DUMB)
	      grub_printf ("\r    Entry %d will be booted automatically in %d seconds.   ", 
			   entryno, grub_timeout);
	  else
	    {
	      int i;
	      char tmp_buf[128];
	      char ch = ' ';

	      grub_sprintf (tmp_buf, " The highlighted entry will be booted automatically in %d seconds.", grub_timeout);
	      gotoxy (MENU_BOX_X - 2, MENU_BOX_H + 7);
	      for (i = 0; i < 79; i++)
	      {
		if (ch)
			ch = tmp_buf[i];
		grub_putchar (ch ? ch : ' ');
	      }
//#ifdef SUPPORT_GRAPHICS
//	      if (! graphics_inited)
//#endif
//	      {
//		gotoxy (MENU_BOX_X - 2, MENU_BOX_H + 8);
//		for (i = 0; i < 79; i++)
//			grub_putchar (' ');
//	      }
	      gotoxy (MENU_BOX_E, MENU_BOX_Y + entryno);
	    }
	  
	  grub_timeout--;
	}

      /* Check for a keypress, however if TIMEOUT has been expired
	 (GRUB_TIMEOUT == -1) relax in GETKEY even if no key has been
	 pressed.  
	 This avoids polling (relevant in the grub-shell and later on
	 in grub if interrupt driven I/O is done).  */
      if (checkkey () >= 0 || grub_timeout < 0)
	{
	  /* Key was pressed, show which entry is selected before GETKEY,
	     since we're comming in here also on GRUB_TIMEOUT == -1 and
	     hang in GETKEY */
	  if (current_term->flags & TERM_DUMB)
	    grub_printf ("\r    Highlighted entry is %d: ", entryno);

	  c = /*ASCII_CHAR*/ (getkey ());

	  if (! old_c_count_end)
	  {
	    if (old_c == 0)
		old_c = c;
	    if (c == old_c && old_c_count < 0x7FFFFFFF)
		old_c_count++;
	    if (c != old_c)
	    {
		old_c_count_end = 1;
	    }
	  }

	  if (grub_timeout >= 0)
	    {
	      int i;

	      if (current_term->setcolorstate)
		  current_term->setcolorstate (COLOR_STATE_HELPTEXT);

	      if (current_term->flags & TERM_DUMB)
		grub_putchar ('\r');
	      else
		gotoxy (MENU_BOX_X - 2, MENU_BOX_H + 7);
	      for (i = 0; i < 79/*158*/; i++)
	      {
//		if (i == 79)
//		{
//#ifdef SUPPORT_GRAPHICS
//			if (graphics_inited)
//				break;
//#endif
//			grub_putchar ('\n');
//		}
		grub_putchar (' ');
	      }
	      grub_timeout = -1;
	      fallback_entryno = -1;
	      if (! (current_term->flags & TERM_DUMB))
		gotoxy (MENU_BOX_E, MENU_BOX_Y + entryno);
	    }

	  if (num_entries == 0)
	    {
		first_entry = entryno = 0;
		goto done_key_handling;
	    }

	  /* We told them above (at least in SUPPORT_SERIAL) to use
	     '^' or 'v' so accept these keys.  */
	  if (c == KEY_UP/*16*/ || c == KEY_LEFT || ((char)c) == '^')
	    {
	      temp_entryno = 0;
	      if (current_term->flags & TERM_DUMB)
		{
		  if (entryno > 0)
		      entryno--;
		}
	      else
		{
		  if (c == KEY_UP)
		  {
		      temp_entryno = first_entry + entryno;
		      for (;;)
		      {
		          temp_entryno = (temp_entryno + num_entries - 1) % num_entries;
		          if (temp_entryno == first_entry + entryno)
			      goto done_key_handling;
		          cur_entry = get_entry (menu_entries, temp_entryno);
		          if (*cur_entry != 0x08)
		              goto check_update;
		      }
		  }

		  if (entryno > 0)
		    {
		      cur_entry = get_entry (menu_entries, first_entry + entryno);
		      /* un-highlight the current entry */
		      print_entry (MENU_BOX_Y + entryno, 0, cur_entry, config_entries);
		      entryno--;
		      cur_entry = get_entry (menu_entries, first_entry + entryno);
		      /* highlight the previous entry */
		      print_entry (MENU_BOX_Y + entryno, 1, cur_entry, config_entries);
		    }
		  else if (first_entry > 0)
		    {
		      first_entry--;
		      print_entries (first_entry, entryno, menu_entries, config_entries);
		    }
		  else	/* loop forward to END */
		    {
		      temp_entryno = num_entries - 1;
		      goto check_update;
		    }
		}
	    }
	  else if ((c == KEY_DOWN/*14*/ || c == KEY_RIGHT || ((char)c) == 'v' || (!old_c_count_end && c == old_c && old_c_count >= 8))
		   /* && first_entry + entryno + 1 < num_entries */)
	    {
	      temp_entryno = 0;
	      if (current_term->flags & TERM_DUMB)
		{
		  if (first_entry + entryno + 1 < num_entries)
		      entryno++;
		}
	      else
		{
		  if (c == KEY_DOWN)
		  {
		      temp_entryno = first_entry + entryno;
		      for (;;)
		      {
		          temp_entryno = (temp_entryno + 1) % num_entries;
		          if (temp_entryno == first_entry + entryno)
			      goto done_key_handling;
		          cur_entry = get_entry (menu_entries, temp_entryno);
		          if (*cur_entry != 0x08)
		              goto check_update;
		      }
		  }

		  if (first_entry + entryno + 1 >= num_entries)
		    {
		      temp_entryno = 0;	/* loop backward to HOME */
		      goto check_update;
		    }
		  if (entryno < MENU_BOX_H - 1)
		    {
		      cur_entry = get_entry (menu_entries, first_entry + entryno);
		      /* un-highlight the current entry */
		      print_entry (MENU_BOX_Y + entryno, 0, cur_entry, config_entries);
		      entryno++;
		      cur_entry = get_entry (menu_entries, first_entry + entryno);
		      /* highlight the next entry */
		      print_entry (MENU_BOX_Y + entryno, 1, cur_entry, config_entries);
		    }
		  else if (num_entries > MENU_BOX_H + first_entry)
		    {
		      first_entry++;
		      print_entries (first_entry, entryno, menu_entries, config_entries);
		    }
		}
	    }
	  else if (c == KEY_PPAGE/*7*/)
	    {
	      /* Page Up */
	      temp_entryno = 0;
	      first_entry -= MENU_BOX_H;
	      if (first_entry < 0)
		{
		  entryno += first_entry;
		  first_entry = 0;
		  if (entryno < 0)
		    entryno = 0;
		}
	      print_entries (first_entry, entryno, menu_entries, config_entries);
	    }
	  else if (c == KEY_NPAGE/*3*/)
	    {
	      /* Page Down */
	      temp_entryno = 0;
	      first_entry += MENU_BOX_H;
	      if (first_entry + entryno + 1 >= num_entries)
		{
		  first_entry = num_entries - MENU_BOX_H;
		  if (first_entry < 0)
		    first_entry = 0;
		  entryno = num_entries - first_entry - 1;
		}
	      print_entries (first_entry, entryno, menu_entries, config_entries);
	    }
	  else if ( ((char)c) >= '0' && ((char)c) <= '9')
	    {
	      temp_entryno *= 10;
	      temp_entryno += ((char)c) - '0';
//	      if (temp_entryno >= num_entries)
//		  temp_entryno = num_entries - 1;
	      if (temp_entryno >= num_entries)	/* too big an entryno */
	      {
		if ((char)c - '0' >= num_entries)
		  temp_entryno = 0;
	        else
		  temp_entryno = (char)c - '0';
	      }
	      if (temp_entryno != 0 || (char)c == '0')
	      {
check_update:
		  if (temp_entryno != first_entry + entryno)
		  {
		      /* check if entry temp_entryno is out of the screen */
		      if (temp_entryno < first_entry || temp_entryno >= first_entry + MENU_BOX_H)
		      {
			  first_entry = (temp_entryno / MENU_BOX_H) * MENU_BOX_H;
			  entryno = temp_entryno % MENU_BOX_H;
			  print_entries (first_entry, entryno, menu_entries, config_entries);
		      } else {
			  /* entry temp_entryno is on the screen, its relative entry number is
 			   * (temp_entryno - first_entry) */
			  cur_entry = get_entry (menu_entries, first_entry + entryno);
			  /* un-highlight the current entry */
			  print_entry (MENU_BOX_Y + entryno, 0, cur_entry, config_entries);
			  entryno = temp_entryno - first_entry;
			  cur_entry = get_entry (menu_entries, temp_entryno);
			  /* highlight entry temp_entryno */
			  print_entry (MENU_BOX_Y + entryno, 1, cur_entry, config_entries);
		      }
		  }
	      }
	      if (temp_entryno * 10 >= num_entries)
		  temp_entryno = 0;
	    }
	  else if (c == KEY_HOME/*1*/)
	    {
	      temp_entryno = 0;
	      goto check_update;
	    }
	  else if (c == KEY_END/*5*/)
	    {
	      temp_entryno = num_entries - 1;
	      goto check_update;
	    }
	  else
	      temp_entryno = 0;

done_key_handling:

	  if (current_term->setcolorstate)
	      current_term->setcolorstate (COLOR_STATE_HEADING);

	  gotoxy (MENU_BOX_E - 4, MENU_BOX_Y - 2);
	  grub_printf ("%3d ", first_entry + entryno);
	  gotoxy (MENU_BOX_E, MENU_BOX_Y + entryno);

	  if (current_term->setcolorstate)
	      current_term->setcolorstate (COLOR_STATE_STANDARD);

	  if (!old_c_count_end && c == old_c && (old_c_count >= 30 || (old_c_count >= 8 && c != KEY_DOWN /*&& c != KEY_RIGHT*/ && c != KEY_UP /*&& c != KEY_LEFT*/)))
		grub_timeout = 5;

	  cur_entry = NULL;

	  if (config_entries)
	    {
	      if ((((char)c) == '\n') || (((char)c) == '\r') || (((char)c) == 'b') || (((char)c) == '#') || (((char)c) == '*'))
		break;
	    }
	  else
	    {
	      if ((((char)c) == 'd') || (((char)c) == 'o') || (((char)c) == 'O'))
		{
		  if (! (current_term->flags & TERM_DUMB))
		    print_entry (MENU_BOX_Y + entryno, 0,
				 get_entry (menu_entries, first_entry + entryno), config_entries);

		  /* insert after is almost exactly like insert before */
		  if (((char)c) == 'o')
		    {
		      /* But `o' differs from `O', since it may causes
			 the menu screen to scroll up.  */
		      if (num_entries > 0)
		      {
			if (entryno < MENU_BOX_H - 1 || (current_term->flags & TERM_DUMB))
			  entryno++;
			else
			  first_entry++;
		      }
		      
		      c = 'O';
		    }

		  cur_entry = get_entry (menu_entries, first_entry + entryno);

		  if (((char)c) == 'O')
		    {
		      grub_memmove (cur_entry + 2, cur_entry, ((int) heap) - ((int) cur_entry));

		      cur_entry[0] = ' ';
		      cur_entry[1] = 0;

		      heap += 2;

		      /*if (first_entry + entryno == num_entries - 1)
		      {
			cur_entry[2] = 0;
			heap++;
		      }*/

		      num_entries++;
		    }
		  else if (num_entries > 0)
		    {
		      char *ptr = get_entry (menu_entries, first_entry + entryno + 1);

		      grub_memmove (cur_entry, ptr, ((int) heap) - ((int) ptr));
		      heap -= (((int) ptr) - ((int) cur_entry));

		      num_entries--;

		      if (/*entryno >= num_entries && */entryno > 0)
			entryno--;
		      else if (first_entry/* && num_entries < MENU_BOX_H + first_entry*/)
			first_entry--;
		    }

		  if (current_term->flags & TERM_DUMB)
		    {
		      grub_putchar ('\n');
		      grub_putchar ('\n');
		      print_entries_raw (num_entries, first_entry, menu_entries);
		    }
		  else if (num_entries > 0)
		    print_entries (first_entry, entryno, menu_entries, config_entries);
		  else
		    print_entry (MENU_BOX_Y, 0, cur_entry, config_entries);
		}

	      cur_entry = menu_entries;
	      if (((char)c) == 27)
		return;
	      if (((char)c) == 'b')
		break;
	    }

	  if (! auth && password)
	    {
	      if (((char)c) == 'p')
		{
		  /* Do password check here! */
		  char entered[32];
		  char *pptr = password;

		  if (current_term->flags & TERM_DUMB)
		    grub_printf ("\r                                    ");
		  else
		    gotoxy (MENU_BOX_X - 2, MENU_BOX_H + 6);

		  /* Wipe out the previously entered password */
		  grub_memset (entered, 0, sizeof (entered));
		  prompt = "Password: ";
		  maxlen = sizeof (entered) - 1;
		  echo_char = '*';
		  readline = 0;
		  get_cmdline (entered);

		  while (! isspace (*pptr) && *pptr)
		    pptr++;

		  /* Make sure that PASSWORD is NUL-terminated.  */
		  *pptr++ = 0;

		  if (! check_password (entered, password, password_type))
		    {
		      char *new_file = config_file;
		      while (isspace (*pptr))
			pptr++;

		      /* If *PPTR is NUL, then allow the user to use
			 privileged instructions, otherwise, load
			 another configuration file.  */
		      if (*pptr != 0)
			{
			  while ((*(new_file++) = *(pptr++)) != 0)
			    ;

			  /* Make sure that the user will not have
			     authority in the next configuration.  */
			  auth = 0;
			  return;
			}
		      else
			{
			  /* Now the user is superhuman.  */
			  auth = 1;
			  goto restart;
			}
		    }
		  else
		    {
		      grub_printf ("Failed! Press any key to continue...");
		      getkey ();
		      goto restart;
		    }
		}
	    }
	  else
	    {
	      if (((char)c) == 'e')
		{
		  int new_num_entries = 0, i = 0;
		  char *new_heap;

		  if (num_entries == 0)
		    {
		      first_entry = entryno = 0;
		      cur_entry = get_entry (menu_entries, 0);
		      grub_memmove (cur_entry + 2, cur_entry, ((int) heap) - ((int) cur_entry));

		      cur_entry[0] = ' ';
		      cur_entry[1] = 0;

		      heap += 2;

		      /*if (first_entry + entryno == num_entries - 1)
		      {
			cur_entry[2] = 0;
			heap++;
		      }*/

		      num_entries++;
		    }

		//  while (first_entry + entryno >= num_entries)
		//  {
		//      if (entryno > 0)
		//	  entryno--;
		//      else
		//	  first_entry--;
		//  }

		  if (config_entries)
		    {
		      new_heap = heap;
		      //cur_entry = get_entry (config_entries, first_entry + entryno, 1);
		      cur_entry = titles[first_entry + entryno];
		      while (*cur_entry++);	/* the first entry */
		    }
		  else
		    {
		      /* safe area! */
		      new_heap = heap + NEW_HEAPSIZE + 1;
		      cur_entry = get_entry (menu_entries, first_entry + entryno);
		    }

		  do
		    {
		      while ((*(new_heap++) = cur_entry[i++]) != 0);
		      new_num_entries++;
		    }
		  while (config_entries && cur_entry[i]);

		  /* this only needs to be done if config_entries is non-NULL,
		     but it doesn't hurt to do it always */
		  *(new_heap++) = 0;

		  if (config_entries && new_num_entries)
		    run_menu (heap, NULL, new_num_entries, new_heap, 0);	/* recursive!! */
		  else
		    {
		      cls ();
		      print_cmdline_message (0);

		      new_heap = heap + NEW_HEAPSIZE + 1;

		      saved_drive = boot_drive;
		      saved_partition = install_partition;
		      current_drive = GRUB_INVALID_DRIVE;

		      prompt = PACKAGE " edit> ";
		      maxlen = NEW_HEAPSIZE + 1;
		      echo_char = 0;
		      readline = 1;
		      if (! get_cmdline (new_heap))
			{
			  int j = 0;

			  /* get length of new command */
			  while (new_heap[j++])
			    ;

			  if (j < 2)
			    {
			      j = 2;
			      new_heap[0] = ' ';
			      new_heap[1] = 0;
			    }

			  /* align rest of commands properly */
			  grub_memmove (cur_entry + j, cur_entry + i, (int) heap - ((int) cur_entry + i));

			  /* copy command to correct area */
			  grub_memmove (cur_entry, new_heap, j);

			  heap += (j - i);
			  if (first_entry + entryno == num_entries - 1)
			  {
				cur_entry[j] = 0;
				heap++;
			  }
			}
		    }

		  goto restart;
		}
	      if (((char)c) == 'c')
		{
		  enter_cmdline (heap, 0);
		  goto restart;
		}
#ifdef GRUB_UTIL
	      if (((char)c) == 'q')
		{
		  /* The same as ``quit''.  */
		  stop ();
		}
#endif
	    }
	}
    }
  
  /* Attempt to boot an entry.  */
  
 boot_entry:
  
  cls ();
  setcursor (1);
//  /* if our terminal needed initialization, we should shut it down
//   * before booting the kernel, but we want to save what it was so
//   * we can come back if needed */
//  prev_term = current_term;
////  if (current_term->shutdown)
////    {
////      (*current_term->shutdown)();
////      current_term = term_table; /* assumption: console is first */
////    }

  fallbacked_entries = 0;
  while (1)
    {
      if (debug > 0)
      {
	if (config_entries)
	{
		char *p;
		char ch;

		p = get_entry (menu_entries, first_entry + entryno);
		//printf ("  Booting \'%s\'\n\n", (((*p) & 0xF0) ? p : ++p));
		if (! ((*p) & 0xF0))
			p++;
		grub_putstr ("  Booting ");
		while ((ch = *p++) && ch != '\n') grub_putchar (ch);
		grub_putchar ('\n');
		grub_putchar ('\n');
	}
	else
		printf ("  Booting command-list\n\n");
      }

      /* Set CURRENT_ENTRYNO for the command "savedefault".  */
      current_entryno = first_entry + entryno;
      
      if (! cur_entry)
      {
	//cur_entry = get_entry (config_entries, first_entry + entryno, 1);
	cur_entry = titles[current_entryno];
	while (*cur_entry++);
      }

      if (! run_script (cur_entry, heap))
	break;
      if (fallback_entryno < 0)
	break;
      cur_entry = NULL;
      first_entry = 0;
      entryno = fallback_entries[fallback_entryno];
      fallback_entryno++;
      if (fallback_entryno >= MAX_FALLBACK_ENTRIES || fallback_entries[fallback_entryno] < 0)
	fallback_entryno = -1;
      fallbacked_entries++;
      if (fallbacked_entries > num_entries * num_entries * 4)
      {
	printf ("\nEndless fallback loop detected(entry=%d)! Press any key to exit...", current_entryno);
	(void) getkey ();
	break;
      }
    }

//  /* if we get back here, we should go back to what our term was before */
//  current_term = prev_term;
//  if (current_term->startup)
//      /* if our terminal fails to initialize, fall back to console since
//       * it should always work */
//      if ((*current_term->startup)() == 0)
//          current_term = term_table; /* we know that console is first */
  show_menu = 1;
  goto restart;
}

// #define GFX_DEBUG

#ifdef SUPPORT_GFX

/* kernel + (grub-)module options */
#define GFX_CMD_BUF_SIZE 512

/* command line separator char */
#define GFX_CMD_SEP 1

/*
 * Search cpio archive for gfx file.
 */
static unsigned find_file (unsigned char *buf, unsigned len,
			   unsigned *gfx_file_start, unsigned *file_len,
			   unsigned *code_start)
{
  unsigned i, fname_len;

  *gfx_file_start = 0;

  for(i = 0; i < len;) 
    {
      if((len - i) >= 0x1a && (buf[i] + (buf[i + 1] << 8)) == 0x71c7)
      {
	fname_len = *(unsigned short *) (buf + i + 20);
	*file_len = *(unsigned short *) (buf + i + 24) + (*(unsigned short *) (buf + i + 22) << 16);
	i += 26 + fname_len;
	i = ((i + 1) & ~1);
	if (*(unsigned *) (buf + i) == 0x0b2d97f00)	/* magic id */
	  {
	    int v = buf[i + 4];

	    *code_start = *(unsigned *) (buf + i + 8);
	    *gfx_file_start = i;
	    if ((v>=5) && (v <= 7))	/* version 5 - 7 */
	      return 1;
	    else if (v == 8)		/* version 8 */
	      return 2;
	  }
	i += *file_len;
	i = ((i + 1) & ~1);
      }
    else
      break;
  }

  return 0;
}

#define MIN_GFX_FREE	0x1000

#define SC_BOOTLOADER		0
#define SC_FAILSAFE		3
#define SC_SYSCONFIG_SIZE	4
#define SC_BOOTLOADER_SEG	8
#define SC_XMEM_0		24
#define SC_XMEM_1		26
#define SC_XMEM_2		28
#define SC_XMEM_3		30
#define SC_FILE			32
#define SC_ARCHIVE_START	36
#define SC_ARCHIVE_END		40
#define SC_MEM0_START		44
#define SC_MEM0_END		48

unsigned long gfx_drive, gfx_partition;

/*
 * Does normally not return.
 */
static void
run_graphics_menu (char *menu_entries, char *config_entries, int num_entries,
	  char *heap, int entryno)
{
  unsigned char *buf, *buf_ext;
  unsigned buf_size, buf_ext_size, code_start, file_start;
  char *cfg;
  int i, j, max_len, gfx_file_size, verbose;
  int selected_entry;
  gfx_data_v1_t *gfx1;
  gfx_data_v2_t *gfx2;
  char *cmd_buf;
  unsigned mem0_start, mem0_end, file_len;
  int version;
  unsigned long tmp_drive, tmp_partition;

  /*
   * check gfx_data_t struct offsets for consistency; gcc will optimize away
   * the whole block
   */

  /* dummy function to make ld fail */
  {
    extern void wrong_struct_size(void);
    #define gfx_ofs_check(a) if(gfx_ofs_v1_##a != (char *) &gfx1->a - (char *) &gfx1->ok) wrong_struct_size();
    gfx_ofs_check(ok);
    gfx_ofs_check(mem_start);
    gfx_ofs_check(mem_cur);
    gfx_ofs_check(mem_max);
    gfx_ofs_check(code_seg);
    gfx_ofs_check(jmp_table);
    gfx_ofs_check(sys_cfg);
    gfx_ofs_check(cmdline);
    gfx_ofs_check(cmdline_len);
    gfx_ofs_check(menu_list);
    gfx_ofs_check(menu_default_entry);
    gfx_ofs_check(menu_entries);
    gfx_ofs_check(menu_entry_len);
    gfx_ofs_check(args_list);
    gfx_ofs_check(args_entry_len);
    gfx_ofs_check(timeout);
    gfx_ofs_check(mem_file);
    gfx_ofs_check(mem_align);
    #undef gfx_ofs_check

    #define gfx_ofs_check(a) if(gfx_ofs_v2_##a != (char *) &gfx2->a - (char *) &gfx2->ok) wrong_struct_size();
    gfx_ofs_check(ok);
    gfx_ofs_check(code_seg);
    gfx_ofs_check(jmp_table);
    gfx_ofs_check(sys_cfg);
    gfx_ofs_check(cmdline);
    gfx_ofs_check(cmdline_len);
    gfx_ofs_check(menu_list);
    gfx_ofs_check(menu_default_entry);
    gfx_ofs_check(menu_entries);
    gfx_ofs_check(menu_entry_len);
    gfx_ofs_check(args_list);
    gfx_ofs_check(args_entry_len);
    gfx_ofs_check(timeout);
    #undef gfx_ofs_check
  }

  if(!num_entries) return;

  kernel_type = KERNEL_TYPE_NONE;

  gfx1 = (gfx_data_v1_t *) heap;
  heap = (char *) (((unsigned) heap + sizeof (*gfx1) + 0xF) & ~0xF);
  gfx2 = (gfx_data_v2_t *) heap;
  heap += sizeof *gfx2;
  memset((char *) gfx1, 0, (char *) heap - (char *) gfx1);
  
  //verbose = (*(unsigned char *) 0x417) & 3 ? 1 : 0;	/* SHIFT pressed */
  verbose = (debug > 1);
  
  /* setup command line edit buffer */

  gfx1->cmdline_len = gfx2->cmdline_len = 256;
  gfx1->cmdline = gfx2->cmdline = heap;
  heap += gfx1->cmdline_len;
  memset(gfx1->cmdline, 0, gfx1->cmdline_len);

  cmd_buf = heap;
  heap += GFX_CMD_BUF_SIZE;

  /* setup menu entries */

  for (i = max_len = 0; i < num_entries; i++)
    {
	char ch;
	char *str = get_entry(menu_entries, i);

	j = 0;
	/* ending at LF, the start of help message */
	while ((ch = *str++) && ch != '\n')
		j++;

	if (max_len < j)
	    max_len = j;
    }

  if (!max_len) return;

  gfx1->menu_entry_len = gfx2->menu_entry_len = max_len;
  gfx1->menu_entries = gfx2->menu_entries = num_entries;

  gfx1->menu_list = gfx2->menu_list = heap;
  heap += gfx1->menu_entry_len * gfx1->menu_entries;

  memset(gfx1->menu_list, 0, gfx1->menu_entry_len * gfx1->menu_entries);

  for(i = 0; i < (int) gfx1->menu_entries; i++)
    {
	char ch;
	char *dest = gfx1->menu_list + i * gfx1->menu_entry_len;
	char *src = get_entry(menu_entries, i);

	/* Skip the leading '\t'.  */
	src++;
	
	/* ending at LF, the start of help message */
	while ((ch = *src++) && ch != '\n')
		*dest++ = ch;
	*dest = 0;
    }

  gfx1->menu_default_entry = gfx2->menu_default_entry = gfx1->menu_list + entryno * gfx1->menu_entry_len;

  gfx1->args_entry_len = gfx2->args_entry_len = 1;
  gfx1->args_list = gfx2->args_list = heap;
  heap += gfx1->args_entry_len * gfx1->menu_entries;
  memset(gfx1->args_list, 0, gfx1->args_entry_len * gfx1->menu_entries);

  /* use 1MB starting at 4MB as file buffer */
  buf_ext = (unsigned char *) (4 << 20);
  buf_ext_size = 1 << 20;

  /* must be 16-byte aligned */
  buf = (unsigned char *) (((unsigned) heap + 0xf) & ~0xf);

  buf_size = ((*((unsigned short *)0x413)) << 10) - (unsigned) buf;
  buf_size &= ~0xf;

  mem0_start = (unsigned) buf;
  mem0_end = mem0_start + buf_size;

#ifdef GFX_DEBUG
  if (verbose)
    {
      printf("low memory 0x%x - 0x%x (%d bytes)\n", mem0_start, mem0_end, buf_size);
    }
#endif

  /* read the file */
  tmp_drive = saved_drive;
  tmp_partition = saved_partition;
  saved_drive = gfx_drive;
  saved_partition = gfx_partition;
  if (!grub_open(graphics_file)) 
    {
      if (verbose)
	printf("%s: file not found\n", graphics_file);

      saved_drive = tmp_drive;
      saved_partition = tmp_partition;
      return;
    }

  gfx_file_size = grub_read ((char *)buf_ext, buf_ext_size, 0xedde0d90);

  grub_close();

  saved_drive = tmp_drive;
  saved_partition = tmp_partition;

  if (gfx_file_size <= 0)
    {
      if (verbose)
	printf("%s: read error\n", graphics_file);

      return;
    }

  /* leave graphics mode now before the extended memory is overwritten. */
#ifdef SUPPORT_GRAPHICS
  if (graphics_inited)
  {
    graphics_end ();
    current_term = term_table; /* assumption: console is first */
  }
#endif

#ifdef GFX_DEBUG
  if (verbose)
    {
      printf("%s: %d bytes (%d bytes left)\n", graphics_file, gfx_file_size, buf_ext_size - gfx_file_size);
    }
#endif

  /* locate file inside cpio archive */
  if (!(version = find_file(buf_ext, gfx_file_size, &file_start, &file_len, &code_start)))
    {
      if (verbose)
	printf("%s: invalid file format\n", graphics_file);

      return;
    }

#ifdef GFX_DEBUG
  if (verbose)
    {
      printf("init: start 0x%x, len %d; code offset 0x%x\n", file_start, file_len, code_start);
    }
#endif

  if (version == 1)
    {
      unsigned u;

      if (gfx_file_size + MIN_GFX_FREE + 0xf >= (int) buf_size)
	{
	  if (verbose)
	    printf("not enough free memory: %d extra bytes need\n", gfx_file_size + MIN_GFX_FREE - buf_size);

	  return;
	}

      memcpy ((void *) buf, (void *) buf_ext, gfx_file_size);

      gfx1->sys_cfg[0] = 2;	/* bootloader: grub */
      gfx1->timeout = grub_timeout >= 0 ? grub_timeout : 0;

      gfx1->mem_start = (unsigned) buf;
      gfx1->mem_max = gfx1->mem_start + buf_size;

      gfx1->mem_cur = gfx1->mem_start +
	((gfx_file_size + 0x0f + 3) & ~3);	/* align it */

      /* align it */
      u = (-(code_start + gfx1->mem_start + file_start)) & 0x0f;
      gfx1->mem_align = gfx1->mem_start + u;
      gfx1->mem_file = gfx1->mem_align + file_start;
      if (u)
	{
	  memcpy((void *) gfx1->mem_align, (void *) gfx1->mem_start, gfx_file_size);
	}

      code_start += gfx1->mem_file;
      gfx1->code_seg = code_start >> 4;
  
      for (i = 0; (unsigned) i < sizeof gfx1->jmp_table / sizeof *gfx1->jmp_table; i++) 
	{
	  gfx1->jmp_table[i] = (gfx1->code_seg << 16) + ((unsigned short *) code_start)[i];
	}
      
      if (gfx_init_v1 (gfx1))
	{
	  if (verbose)
	    printf("graphics initialization failed\n");

	  return;
	}
      
      gfx_setup_menu_v1 (gfx1);
      i = gfx_input_v1 (gfx1, &selected_entry);
      gfx_done_v1 (gfx1);
    }
  else
    {
      if (file_len - code_start + MIN_GFX_FREE > buf_size)
	{
	  if (verbose)
	    printf("not enough free memory: %d extra bytes need\n", file_len - code_start + MIN_GFX_FREE - buf_size);

	  return;
	}

      gfx2->sys_cfg[SC_BOOTLOADER] = 2;			/* bootloader: grub */
      gfx2->sys_cfg[SC_SYSCONFIG_SIZE] = 52;		/* config data size */
      *(unsigned short *) (gfx2->sys_cfg + SC_BOOTLOADER_SEG) = (unsigned) gfx2 >> 4;	/* segment */
      gfx2->sys_cfg[SC_XMEM_0] = 0x21;			/* 1MB @ 2MB */
      gfx2->sys_cfg[SC_XMEM_1] = 0x41;			/* 1MB @ 4MB */
      gfx2->sys_cfg[SC_FAILSAFE] = verbose;
      gfx2->timeout = grub_timeout >= 0 ? grub_timeout : 0;

      memcpy((void *) buf, (void *) (buf_ext + file_start + code_start), file_len - code_start);

      mem0_start += file_len - code_start;
      mem0_start = (mem0_start + 3) & ~3;		/* align */

      /* init interface to graphics functions */

      *(unsigned *) (gfx2->sys_cfg + SC_FILE) = (unsigned) buf_ext + file_start;
      *(unsigned *) (gfx2->sys_cfg + SC_ARCHIVE_START) = (unsigned) buf_ext;
      *(unsigned *) (gfx2->sys_cfg + SC_ARCHIVE_END) = (unsigned) buf_ext + gfx_file_size;
      *(unsigned *) (gfx2->sys_cfg + SC_MEM0_START) = mem0_start;
      *(unsigned *) (gfx2->sys_cfg + SC_MEM0_END) = mem0_end;

      gfx2->code_seg = (unsigned) buf >> 4;

#ifdef GFX_DEBUG

      if (verbose)
	{
	  printf("init 0x%x, archive 0x%x - 0x%x, low mem 0x%x - 0x%x\ncode seg 0x%x\n",
		 (unsigned) buf_ext + file_start,
		 (unsigned) buf_ext, (unsigned) buf_ext + gfx_file_size,
		 mem0_start, mem0_end, gfx2->code_seg
	  );
	}
#endif

      for (i = 0; (unsigned) i < sizeof gfx2->jmp_table / sizeof *gfx2->jmp_table; i++)
	{
	  gfx2->jmp_table[i] = (gfx2->code_seg << 16) + ((unsigned short *) buf)[i];
	}

#ifdef GFX_DEBUG
      if (verbose)
	{
	  for(i = 0; i < 12; i++)
	    {
	      printf("%d: 0x%x\n", i, gfx2->jmp_table[i]);
	    }

	  for(i = 0; i < gfx2->menu_entries; i++) 
	    {
	      printf("\"%s\"  --  \"%s\"\n",
		     gfx2->menu_list + i * gfx2->menu_entry_len,
		     gfx2->args_list + i * gfx2->args_entry_len
	      );
	    }

	  printf("default: \"%s\"\n", gfx2->menu_default_entry);
	}
#endif

      /* switch to graphics mode */
      if (gfx_init_v2 (gfx2))
	{
	  if (verbose)
	    printf("graphics initialization failed\n");

	  return;
	}

      gfx_setup_menu_v2 (gfx2);
      i = gfx_input_v2 (gfx2, &selected_entry);
      gfx_done_v2 (gfx2);
    }

  memset ((char *) HISTORY_BUF, 0, HISTORY_BUFLEN);
  /* ESC -> show text menu */

  if (i == 1)
    {
      grub_timeout = -1;
      return;
    }

  //heap = saved_heap;	/* free most of the graphics data */

  // printf("cmdline: >%s<, entry = %d\n", gfx_data->cmdline, selected_entry);

  if (selected_entry < 0 || selected_entry > num_entries)
    return;

  /* for 'savedefault' */
  current_entryno = selected_entry;

  cfg = get_entry(menu_entries, selected_entry);
  while (*(cfg++))
    ;

  run_script(cfg, heap);
}

#endif /* SUPPORT_GFX */

static int
get_line_from_config (char *cmdline, int max_len, int preset)
{
    unsigned long pos = 0, info = 0;//literal = 0, comment = 0;
    char c;  /* since we're loading it a byte at a time! */
  
    while (1)
    {
#if 0
	/* Skip UTF-8 Byte Order Mark: EF BB BF */
	{
	    unsigned long menu_offset;

	    menu_offset = read_from_file ? filepos : preset_menu_offset;
	    if (menu_offset == 0)
	    {
		/* read 3 bytes, check UTF-8 Byte Order Mark: EF BB BF */
		if (read_from_file)
		{
			if (3 != grub_read (&menu_offset, 3))
			{
				filepos = 0;
				break;
			}
			if (menu_offset != 0xBFBBEF)
			{
				filepos = 0;
			}
		}
		else
		{
			if (3 != read_from_preset_menu (&menu_offset, 3))
			{
				preset_menu_offset = 0;
				break;
			}
			if (menu_offset != 0xBFBBEF)
			{
				preset_menu_offset = 0;
			}
		}
	    }
	}
#endif

	if (preset)
	{
	    if (! read_from_preset_menu (&c, 1))
		break;
	}
	else
	{
	    if (! grub_read (&c, 1, 0xedde0d90))
		break;
	}

	///* Skip UTF-8 Byte Order Mark: EF BB BF */
	//if ((c & 0x80) && pos < 3)
	//    continue;

	/* Replace CR with LF.  */
	if (c == '\r')
	    c = '\n';

	/* Replace tabs with spaces.  */
	if (c == '\t')//( || c == '\f' || c == '\v')
	    c = ' ';

	/* all other non-printable chars are illegal. */
	if (c != '\n' && (unsigned char)c < ' ')
	    break;

//	/* The previous is a backslash, then...  */
//	if (info & 1)	/* bit 0 for literal */
//	{
//	    /* If it is a newline, replace it with a space and continue.  */
//	    if (c == '\n')
//	    {
//		c = ' ';
//
//		/* Go back to overwrite a backslash.  */
//		if (pos > 0)
//		    pos--;
//	    }
//
//	    info &= 0xFFFFFFFE;	//literal = 0;
//	}
//
//	///* Replace semi-colon with LF.  */
//	//if (c == ';')
//	//    c = '\n';
//
//	/* translate characters first! */
//	if (c == '\\')
//	    info |= 1;	//literal = 1;

	if (info & 2)	/* bit 1 for comment */
	{
	    if (c == '\n')
		info &= 0xFFFFFFFD;	//comment = 0;
	    /* Skip all comment chars upto end of line. */
	}
	else if (! pos)
	{
	    /* At the very beginning of the line... */
	    if (c == '#')
	    {
		info |= 2;	//comment = 1;
		/* Skip the comment char. */
	    }
	    else
	    {
		/* Skip non-printable chars, including the UTF-8 Byte Order Mark: EF BB BF */
		if ((unsigned char)c > ' ' && (unsigned char)c <= 0x7F) //((c != ' ') && (c != '\t') && (c != '\n') && (c != '\r'))
		    cmdline[pos++] = c;
	    }
	}
	else
	{
	    if (c == '\n')
		break;

	    if (!(info & 4) && ((c & 0x80) || pos > 31))	/* bit 2 for argument */
		break;

	    if (pos < max_len)
	    {
		if (!(info & 4) && c == '=')
		    c = ' ';
		if (c == ' ')
		    info |= 4;	//argument = 1;
		cmdline[pos++] = c;
	    }
	}
    }

    cmdline[pos] = 0;

    return pos;
}

//void
//reset (void);
int config_len;
static int num_entries;
static char *config_entries, *cur_entry;

static void
reset (void)
{
  count_lines = -1;
  config_len = 0;
  num_entries = 0;
  cur_entry = config_entries;

  /* Initialize the data for the configuration file.  */
  default_entry = 0;
  password = 0;
  fallback_entryno = -1;
  fallback_entries[0] = -1;
  grub_timeout = -1;
}
  
static unsigned long attr = 0;

/* This is the starting function in C.  */
void
cmain (void)
{
#ifdef GRUB_UTIL
    /* Initialize the environment for restarting Stage 2.  */
    grub_setjmp (restart_env);
#endif /* GRUB_UTIL */
  
#ifndef GRUB_UTIL
    debug = debug_boot + 1;
#endif /* ! GRUB_UTIL */
  
    titles = (char * *)init_free_mem_start;
    config_entries = ((char *) init_free_mem_start) + 256 * sizeof (char *);

    /* Never return.  */
restart:
    reset ();
      
    /* Here load the configuration file.  */
      
    if (! use_config_file)
	goto done_config_file;

    /* Get a saved default entry if possible.  */
    saved_entryno = 0;
    if (*config_file && boot_drive != cdrom_drive)
    {
	char *default_file = (char *) DEFAULT_FILE_BUF;

	*default_file = 0;	/* initialise default_file */
	grub_strncat (default_file, config_file, DEFAULT_FILE_BUFLEN);
	{
	    int i;
	    for (i = grub_strlen (default_file); i >= 0; i--)
		if (default_file[i] == '/')
			break;
	    default_file[++i] = 0;
	    grub_strncat (default_file + i, "default", DEFAULT_FILE_BUFLEN - i);
	}
	if (debug > 1)
	    grub_printf("Open %s ... ", default_file);
	DEBUG_SLEEP

	if (grub_open (default_file))
	{
	    char buf[10]; /* This is good enough.  */
	    char *p = buf;
	    int len;
	  
	    if (debug > 1)
		grub_printf("Read file: ", default_file);
	    len = grub_read (buf, sizeof (buf), 0xedde0d90);
	    if (debug > 1)
		grub_printf("len=%d\n", len);
	    if (len > 0)
	    {
		buf[sizeof (buf) - 1] = 0;
		safe_parse_maxint (&p, &saved_entryno);
	    }

	    grub_close ();
	}
	else if (debug > 1)
	    grub_printf("failure.\n", default_file);
	DEBUG_SLEEP
    }
    errnum = ERR_NONE;

#ifndef GRUB_UTIL
    pxe_restart_config = 0;
#endif /* ! GRUB_UTIL */

#ifndef GRUB_UTIL
restart_config:
#endif /* ! GRUB_UTIL */

    {
	/* STATE 0: Menu init, i.e., before any title command.
	   STATE 1: In a title command.
	   STATE 2: In a entry after a title command.  */
	int state = 0, prev_config_len = 0;
	char *cmdline;
	int is_preset;

	{
	    int is_opened;

	    is_preset = is_opened = 0;
	    /* Try command-line menu first if it is specified. */
	    if (preset_menu == (char *)0x0800 /*&& ! *config_file*/)
	    {
		is_opened = is_preset = open_preset_menu ();
	    }
	    if (! is_opened)
	    {
		/* Try config_file */
#ifndef GRUB_UTIL
		if (*config_file)
			is_opened = (configfile_opened || grub_open (config_file));
#else
		if (*config_file)
			is_opened = grub_open (config_file);
#endif /* ! GRUB_UTIL */
	    }
	    errnum = ERR_NONE;
#ifndef GRUB_UTIL
	    configfile_opened = 0;
#endif /* ! GRUB_UTIL */
	    if (! is_opened)
	    {
#ifndef GRUB_UTIL
		if (pxe_restart_config)
			goto original_config;
#endif /* ! GRUB_UTIL */

		/* Try the preset menu. This will succeed at most once,
		 * because the preset menu will be disabled(see below).  */
		is_opened = is_preset = open_preset_menu ();
	    }

	    if (! is_opened)
		goto done_config_file;
	}

	/* This is necessary, because the menu must be overrided.  */
	reset ();
	cmdline = (char *) CMDLINE_BUF;
	  
	while (get_line_from_config (cmdline, NEW_HEAPSIZE, is_preset))
	{
	    struct builtin *builtin;
	  
	    /* Get the pointer to the builtin structure.  */
	    builtin = find_command (cmdline);
	    errnum = 0;
	    if (! builtin)
		/* Unknown command. Just skip now.  */
		continue;
	  
	    if (builtin->flags & BUILTIN_TITLE)	/* title command */
	    {
	        /* Finish the menu init commands or previous menu items.  */

		if (state == 2)
		{
		    /* The next title is found.  */
		    if (num_entries >= 256)
			break;
		    num_entries++;	/* an entry is completed. */
		    config_entries[config_len++] = 0;	/* finish the entry. */
		    prev_config_len = config_len;
		}
		else if (state)		/* state == 1 */
		{
		    /* previous is an invalid title, overwrite it.  */
		    config_len = prev_config_len;
		}
		else			/* state == 0 */
		{
		    /* The first title. So finish the menu init commands. */
		    config_entries[config_len++] = 0;
		}

		/* Reset the state.  */
		state = 1;

		/* Copy title into config area.  */
		{
		    int len;
		    char *ptr = cmdline;
		    while (*ptr && *ptr != ' ' && *ptr != '\t' && *ptr != '=')
			ptr++;
		    if (*ptr)
			ptr++;
		    attr = config_len;
		    if (num_entries < 256)
			titles[num_entries] = config_entries + config_len;
		    config_entries[config_len++] = 0x08;	/* attribute byte */
		    len = parse_string (ptr);
		    ptr[len] = 0;
		    while ((config_entries[config_len++] = *(ptr++)) != 0);
		}
	    }
	    else if (! state)			/* menu init command */
	    {
		if (builtin->flags & BUILTIN_MENU)
		{
		    char *ptr = cmdline;
		    /* Copy menu-specific commands to config area.  */
		    while ((config_entries[config_len++] = *ptr++) != 0);
		    prev_config_len = config_len;
		}
		else
		    /* Ignored.  */
		    continue;
	    }
	    else				/* menu item command */
	    {
		state = 2;

		/* Copy config file data to config area.  */
		{
		    char *ptr = cmdline;
		    while ((config_entries[config_len++] = *ptr++) != 0);
		    prev_config_len = config_len;
		}
		config_entries[attr] |= !!(builtin->flags & BUILTIN_BOOTING);
	    }
	} /* while (get_line_from_config()) */

	/* file must be closed here, because the menu-specific commands
	 * below may also use the GRUB_OPEN command.  */
	if (is_preset)
	    preset_menu = 0;	/* Disable the preset menu.  */
	else
	    grub_close ();

	if (state == 2)
	{
	    if (num_entries < 256)
	        num_entries++;	/* the last entry is completed. */
	}
	else// if (state)
	{
	    config_len = prev_config_len;
	}

	/* Finish the last entry or the menu init commands.  */
	config_entries[config_len++] = 0;
	if (num_entries < 256)
		titles[num_entries] = 0;
	  
	/* old MENU_BUF is not used any more. So MENU_BUF is a temp area,
	 * and can be moved to elsewhere. */

	/* config_entries contains these:
	 * 1. The array of menu init commands.
	 * 2. The array of menu item commands with leading titles.
	 */

	/* Run menu-specific commands before any other menu entry commands.  */

	{
	    char *old_entry;
	    char *heap = config_entries + config_len;

#ifndef GRUB_UTIL
	    int old_debug = 1;
#endif /* ! GRUB_UTIL */

	    /* Initialize the data.  */
	    //saved_drive = boot_drive;
	    //saved_partition = install_partition;
	    current_drive = GRUB_INVALID_DRIVE;
	    count_lines = -1;
  
	    kernel_type = KERNEL_TYPE_NONE;
	    errnum = 0;

	    while (1)
	    {
		struct builtin *builtin;
		char *arg;
		grub_error_t errnum_old;

#ifndef GRUB_UTIL
		pxe_restart_config = 0;
#endif /* ! GRUB_UTIL */

#ifdef SUPPORT_GFX
		*graphics_file = 0;
#endif

#if 0
#ifndef GRUB_UTIL
		/* pxe_detect should be done before any other command. */
		if (! *config_entries)
		{
			if (pxe_entry && ! pxe_inited)
			{
			    if (pxe_detect (0, 0))
				goto restart_config;
			}
		}
#endif /* ! GRUB_UTIL */
#endif

		errnum_old = errnum;
		errnum = 0;

		/* Copy the first string in CUR_ENTRY to HEAP.  */
		old_entry = cur_entry;
		while (*cur_entry++);

		grub_memmove (heap, old_entry, (int) cur_entry - (int) old_entry);
		if (! *heap)
		{
		    /* If there is no more command in SCRIPT...  */

		    /* If no kernel is loaded, just exit successfully.  */
		    if (kernel_type == KERNEL_TYPE_NONE)
			break;

		    /* Otherwise, the command boot is run implicitly.  */
		    grub_memmove (heap, "boot", 5);
		}

		/* Find a builtin.  */
		builtin = find_command (heap);
		if (! builtin)
		{
		    grub_printf ("%s\n", old_entry);
		    continue;
		}

#if 0
#ifndef GRUB_UTIL
		/* pxe_detect should be done before any other command. */
		if ((builtin->func) != pxe_func && old_entry == config_entries)
		{
			if (pxe_entry && ! pxe_inited)
			{
			    if (pxe_detect (0, 0))
				goto restart_config;
			}
		}
#endif /* ! GRUB_UTIL */
#endif

		/* If BUILTIN cannot be run in the menu, skip it.  */
		if (! (builtin->flags & BUILTIN_MENU))
		{
		    continue;
		}

		if ((builtin->func) == errnum_func && (builtin->func) == checkrange_func)
		    errnum = errnum_old;

		/* find && and || */

		for (arg = skip_to (0, heap); *arg != 0; arg = skip_to (0, arg))
		{
		    struct builtin *builtin1;
		    int ret;
		    char *arg1;
		    arg1 = arg;
		    if (*arg == '&' && arg[1] == '&' && (arg[2] == ' ' || arg[2] == '\t'))
		    {
			/* handle the AND operator */
			arg = skip_to (0, arg);
			builtin1 = find_command (arg);
			if (! builtin1 || ! (builtin1->flags & BUILTIN_MENU))
			{
				errnum = ERR_UNRECOGNIZED;
				goto next;
			}

			*arg1 = 0;
			ret = (builtin->func) (skip_to (1, heap), BUILTIN_MENU);
			*arg1 = '&';
			if (ret)
			{
				arg = skip_to (1, arg);
				if ((builtin1->func) != errnum_func && (builtin1->func) != checkrange_func)
					errnum = 0;
				(builtin1->func) (arg, BUILTIN_MENU);
			} else
				errnum = 0;
			goto next;
		    } else if (*arg == '|' && arg[1] == '|' && (arg[2] == ' ' || arg[2] == '\t'))
		    {
			/* handle the OR operator */
			arg = skip_to (0, arg);
			builtin1 = find_command (arg);
			if (! builtin1 || ! (builtin1->flags & BUILTIN_MENU))
			{
				errnum = ERR_UNRECOGNIZED;
				goto next;
			}

			*arg1 = 0;
			ret = (builtin->func) (skip_to (1, heap), BUILTIN_MENU);
			*arg1 = '|';
			if (! ret)
			{
				arg = skip_to (1, arg);
				if ((builtin1->func) != errnum_func && (builtin1->func) != checkrange_func)
					errnum = 0;
				(builtin1->func) (arg, BUILTIN_MENU);
			} else
				errnum = 0;
			goto next;
		    }
		}
      
		/* Run BUILTIN->FUNC.  */
		arg = (builtin->func) == commandline_func ? heap : skip_to (1, heap);
		(builtin->func) (arg, BUILTIN_MENU);

		/* if the INSERT key was pressed at startup, debug is not allowed to be turned off. */
#ifndef GRUB_UTIL
		if (debug_boot)
		    if ((unsigned int)debug < 2)	/* debug == 0 or 1 */
		    {
			old_debug = debug;	/* save the new debug in old_debug */
			debug = 2;
		    }
#endif /* ! GRUB_UTIL */

next:
#ifdef SUPPORT_GFX
		if (num_entries && ! errnum && *graphics_file && !password && show_menu && grub_timeout)
		{
		  run_graphics_menu((char *)titles, cur_entry, num_entries, config_entries + config_len, default_entry);
		}
#endif
		DEBUG_SLEEP

#ifndef GRUB_UTIL
		if (pxe_restart_config)
			goto restart_config;
#endif /* ! GRUB_UTIL */

#ifndef GRUB_UTIL
original_config:
#endif /* ! GRUB_UTIL */

		if (! *old_entry)
		    break;
	    } /* while (1) */

	    kernel_type = KERNEL_TYPE_NONE;

#ifndef GRUB_UTIL
	    if (debug_boot)
	    {
		debug = old_debug;
		grub_printf ("\n\nEnd of menu init commands. Press any key to enter command-line or run menu...", old_entry);
	    }
#endif /* ! GRUB_UTIL */
	    DEBUG_SLEEP
	}

	/* End of menu-specific commands.  */

	errnum = 0;

	/* Make sure that all fallback entries are valid.  */
	if (fallback_entryno >= 0)
	{
	    int i;

	    for (i = 0; i < MAX_FALLBACK_ENTRIES; i++)
	    {
		if (fallback_entries[i] < 0)
		    break;
		if (fallback_entries[i] >= num_entries)
		{
		    grub_memmove (fallback_entries + i, fallback_entries + i + 1, ((MAX_FALLBACK_ENTRIES - i - 1) * sizeof (int)));
		    i--;
		}
	    }

	    if (fallback_entries[0] < 0)
		fallback_entryno = -1;
	}

	/* Check if the default entry is present. Otherwise reset it to
	   fallback if fallback is valid, or to DEFAULT_ENTRY if not.  */
	if (default_entry >= num_entries)
	{
	    if (fallback_entryno >= 0)
	    {
		default_entry = fallback_entries[0];
		fallback_entryno++;
		if (fallback_entryno >= MAX_FALLBACK_ENTRIES || fallback_entries[fallback_entryno] < 0)
		    fallback_entryno = -1;
	    }
	    else
		default_entry = 0;
	}
    }

done_config_file:

#ifndef GRUB_UTIL
	pxe_restart_config = 1;	/* pxe_detect will use configfile to run menu */
#endif /* ! GRUB_UTIL */

    /* go ahead and make sure the terminal is setup */
    if (current_term->startup)
	(*current_term->startup)();

    if (! num_entries)
    {
	/* no config file, goto command-line, starting heap from where the
	   config entries would have been stored if there were any.  */
	enter_cmdline (config_entries, 1);
    }
    else
    {
//#ifdef SUPPORT_GFX
//      if (*graphics_file && !password && show_menu && grub_timeout)
//	{
//	  run_graphics_menu((char *)titles, cur_entry, num_entries, config_entries + config_len, default_entry);
//	}
//#endif
	/* Run menu interface.  */
	/* cur_entry point to the first menu item command. */
	run_menu ((char *)titles, cur_entry, num_entries, config_entries + config_len, default_entry);
    }
    goto restart;
}

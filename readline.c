char *command_generator __P((const char *, int));
char *
dupstr (s)
     int s;
{
  char *r;

  r = xmalloc (strlen (s) + 1);
  strcpy (r, s);
  return (r);
}


char **
fileman_completion (text, start, end)
     const char *text;
     int start, end;
{
  char **matches;

  matches = (char **)NULL;

  /* If this word is at the start of the line, then it is a command
     to complete.  Otherwise it is the name of a file in the current
     directory. */
  if (start == 0)
    matches = rl_completion_matches (text, command_generator);
  else {
 	struct mycmd *pcmd;
	char *av[20];
	int argc;
	static char buf[256];
	strncpy(buf,rl_line_buffer,256);
	argc = argvize(av,buf);
	if(start != end) argc--;

	for(pcmd = mycmd_head;pcmd && strcmp(pcmd->name,av[0]);pcmd = pcmd->next);
	if(pcmd && pcmd->complete)
	 matches = pcmd->complete(argc,av,text,start,end);
  }

  return (matches);
}
/* Generator function for command completion.  STATE lets us
   know whether to start from scratch; without any state
   (i.e. STATE == 0), then we start at the top of the list. */
char *
command_generator (text, state)
     const char *text;
     int state;
{
  static int list_index, len;
  char *name;
 static struct mycmd *pcmd;

  /* If this is a new word to complete, initialize now.  This
     includes saving the length of TEXT for efficiency, and
     initializing the index variable to 0. */
  if (!state)
    {
	  pcmd = mycmd_head;
      len = strlen (text);
    }

  /* Return the next name which partially matches from the
     command list. */



  while (pcmd && (name = pcmd->name))
    {
	  pcmd = pcmd->next;

      if (strncmp (name, text, len) == 0)
        return (dupstr(name));
    }

  /* If no names matched, then return NULL. */
  return ((char *)NULL);

}

void initialize_readline ()
{
  /* Allow conditional parsing of the ~/.inputrc file. */
	rl_readline_name = "FileMan";
  /* Tell the completer that we want a crack first. */
    rl_attempted_completion_function = fileman_completion;
}

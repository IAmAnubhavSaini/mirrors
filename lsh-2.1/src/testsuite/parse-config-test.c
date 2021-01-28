#include "testutils.h"

#include "parse_config.h"

/* Get private declarations */
#define GABA_DECLARE
#include "parse_config.c.x"
#undef GABA_DECLARE

/* Example configureation from David Kågedal. */
const char config_file[] =
" lysator {\n"
"     name        Lysator\n"
"     user        davidk\n"
"     domain      lysator.liu.se\n"
"     hosts {\n"
"         proton fafner aguilera britney sandra hanna\n"
"         lenin venom\n"
"         mogheiden asmodean\n"
"         nbcd graycode \n"
"     }\n"
" }\n"
" \n"
" kagedal {\n"
"     name        kagedal.org\n"
"     user        davidk\n"
"     domain      zoo.kagedal.org\n"
"     hosts {\n"
"         abakus {\n"
"             address foo31-152.visit.se\n"
"         }\n"
"         kosmos\n"
"     }\n"
" }\n";

#define CONFIG_TYPES 2

static void
display_settings(struct config_setting *setting)
{
  if (!setting)
    werror("    {}\n");
  else
    {
      werror("    {\n");
      for (; setting; setting = setting->next)
	{
	  static const char *keys[CONFIG_TYPES]
	    = { "address", "user" };
	  if (setting->type >= CONFIG_TYPES)
	    werror("      Invalid config type: %i\n", setting->type);
	  else if (!setting->value)
	    werror("      %z = <NULL>\n", keys[setting->type]);
	  else
	    werror("      %z = %S\n", keys[setting->type], setting->value);
	}
      werror("    }\n");
    }
}

static void
display_hosts(struct config_host *host)
{
  if (!host)
    werror("  hosts {}\n");
  else
    {
      werror("  hosts {\n");

      for (; host; host = host->next)
	{
	  if (host->name)
	    werror("    %S\n", host->name);
	  else
	    werror("    <unnamed>\n");
	  display_settings(host->settings);
	}
      werror("  }\n");
    }	    
}

static void
display_groups(struct config_group *group)
{
  for (; group; group = group->next)
    {
      if (group->name)
	werror("%S {\n", group->name);
      else
	werror("<unnamed> {\n");
      display_settings(group->settings);
      display_hosts(group->hosts);
    }
}

int
test_main(void)
{
  struct config_group *config
    = config_parse_string(strlen(config_file), config_file);

  if (!config)
    FAIL();

  display_groups(config);
  
  SUCCESS();
}

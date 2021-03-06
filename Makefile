# Top make file of DABC project

DABCMAINMAKE = true

#debug=4
#extrachecks = true

DABCSYS := $(CURDIR)

include config/Makefile.config

## disable possibility, that rules included many times 
Dabc_Makefile_rules = true

CREATE_DIRS += $(DABCDLLPATH) $(DABCBINPATH) $(DABCINCPATH)


include base/Makefile.mk

DABC_PLUGINS = $(wildcard plugins/*)

DABC_PLUGINS += $(wildcard applications/*)

-include $(patsubst %, %/Makefile, $(DABC_PLUGINS))

-include doc/Makefile

libs:: dabclogin

clean:: clean-doxy
	@$(RM) $(DABCSYS)/config/Makefile.gener

docs:: doxy

doxy:
	@echo "Creating doxygen documentation"
	doxygen doc/DoxygenConfig

clean-doxy:
	@echo "Clean doxygen documentation"
	@$(RMDIR) html

dabclogin: build/dabclogin.sh config/Makefile.config
	@sed -e "s|\`pwd\`|$(CURDIR)|" -e "s|version|$(DABC_VERSION)|" \
	< build/dabclogin.sh > dabclogin; chmod 755 dabclogin; echo "Create dabclogin"

Dabc_Makefile_rules :=

-include config/Makefile.packaging

include config/Makefile.rules

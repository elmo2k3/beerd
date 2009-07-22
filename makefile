DIRS=src

beerd: beerd
%:
	@for DIR in $(DIRS) ; do \
		$(MAKE) -C $$DIR $@; \
	done

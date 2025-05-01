# this file contains common rules
.PHONY: clean
.PHONY: subdirs $(SUBDIRS)
.PHONY: appdirs $(APPDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@ $(MAKECMDGOALS)

$(APPDIRS):
	@$(MAKE) -C $@ $(MAKECMDGOALS)

dist:
	perl DistributionMaker.pl

clean:
	rm -f *~ $(objs) $(extraobjs) $(deps) $(TARGET) 
ifneq ($(TARGET),)
	rm -f $(TOPDIR)/build/$(TARGET)
endif

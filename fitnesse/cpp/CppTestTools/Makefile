all:
	$(MAKE) -C Concurrency all
	$(MAKE) -C Helpers all
	$(MAKE) -C Platforms/Gcc all
	$(MAKE) -C UnitTestHarness all
	$(MAKE) -C Fit/FitLib all
	$(MAKE) -C AllTests all
 
test: all
	./AllTests/AllTests -r
 
clean:
	$(MAKE) -C Concurrency clean
	$(MAKE) -C Helpers clean
	$(MAKE) -C Platforms/Gcc clean
	$(MAKE) -C UnitTestHarness clean
	$(MAKE) -C Fit/FitLib clean
	$(MAKE) -C AllTests clean
	rm -f lib/*.a

depend:
	$(MAKE) -C Concurrency depend
	$(MAKE) -C Helpers depend
	$(MAKE) -C Platforms/Gcc depend
	$(MAKE) -C UnitTestHarness depend
	$(MAKE) -C Fit/FitLib depend
	$(MAKE) -C AllTests depend
	
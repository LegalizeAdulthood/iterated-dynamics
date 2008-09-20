all:
	$(MAKE) -C UtilLib all
	$(MAKE) -C HomeGuardLib all
	$(MAKE) -C MockHardwareLib all
	$(MAKE) -C HomeGuardFixturesLib all
	$(MAKE) -C HomeGuardFitnesseServer all
	$(MAKE) -C AllTests all test
 
test: all
	$(MAKE) -C AllTests test
 
clean:
	$(MAKE) -C UtilLib clean
	$(MAKE) -C HomeGuardLib clean
	$(MAKE) -C MockHardwareLib clean
	$(MAKE) -C HomeGuardFixturesLib clean
	$(MAKE) -C HomeGuardFitnesseServer clean
	$(MAKE) -C AllTests clean
	rm -f lib/*.a
	rm -f test/*.*

depend:
	$(MAKE) -C UtilLib depend
	$(MAKE) -C HomeGuardLib depend
	$(MAKE) -C MockHardwareLib depend
	$(MAKE) -C HomeGuardFixturesLib depend
	$(MAKE) -C HomeGuardFitnesseServer depend
	$(MAKE) -C AllTests depend

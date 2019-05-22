CFLAGS=-lF1X -lf1xRepair -lrt -lboost_program_options -lboost_log -lboost_log_setup -lboost_date_time -lboost_system -lboost_filesystem -lboost_thread -lboost_chrono -lboost_atomic -lpthread

profile: profile.c
	gcc -o profile profile.c $(CFLAGS)

f1x-aflgo: f1x-aflgo.c
	gcc -o f1x-aflgo f1x-aflgo.c $(CFLAGS)

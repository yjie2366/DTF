BINARY=writer
SRC=$(wildcard *.f90)
MOD=$(wildcard *.mod)
FC=mpifrtpx
PNETCDF=/home/g9300001/u93005/dtf/install
DTF=/home/g9300001/u93005/dtf/DTF/libdtf
FLAGS=-I$(PNETCDF)/include -I$(DTF) -I.
LDFLAGS= -L$(PNETCDF)/lib -L$(DTF) -Wl,-rpath=$(DTF) -Wl,-rpath=$(PNETCDF)/lib
LIBS= -lpnetcdf -ldtf

RM=rm

.PHONY: all module clean

all: $(BINARY)

$(BINARY): module
	$(FC) $(SRC) $(FLAGS) $(LDFLAGS) $(LIBS) -o $@

module:
	$(FC) -c $(SRC) $(FLAGS)

clean:
	$(RM) -f $(BINARY)

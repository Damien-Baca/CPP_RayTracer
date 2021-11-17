DPATH=./drivers
IPATH=./images
DIF=diff.txt
FLAGS=-Wall -ansi -pedantic -std=c++11 -c

SRC=raytracer.cpp
OBJ=raytracer.o
EXE=rayTracer.exe
TAR=rayTracer.tar
DRIVER_1=driver00.txt
DRIVER_2=driver01.txt
DRIVER_3=driver02.txt
IMAGE_1=driver00_result.ppm
IMAGE_2=driver01_result.ppm
IMAGE_3=driver02_result.ppm

test: $(IPATH)
	diff $(IPATH)/$(IMAGE_1) $(DPATH)/master_$(IMAGE_1) > $(DIF)
	diff $(IPATH)/$(IMAGE_2) $(DPATH)/master_$(IMAGE_2) >> $(DIF)
	diff $(IPATH)/$(IMAGE_3) $(DPATH)/master_$(IMAGE_3) >> $(DIF)
	cat $(DIF) # Printing differences with master result
	# If nothing else is printed above, test was successful.
$(IPATH): $(EXE)
	mkdir $(IPATH)
	./$(EXE) $(DPATH)/$(DRIVER_1) $(IPATH)/$(IMAGE_1)
	./$(EXE) $(DPATH)/$(DRIVER_2) $(IPATH)/$(IMAGE_2)
	./$(EXE) $(DPATH)/$(DRIVER_3) $(IPATH)/$(IMAGE_3)
$(EXE): $(OBJ)
	g++ -o $(EXE) $(OBJ)
$(OBJ): $(SRC)
	g++ $(FLAGS) $(SRC)

tar:
	tar -cvf $(TAR) drivers Eigen ./makefile ./$(SRC) ./README.txt
clean:
	rm $(EXE) $(TAR) $(OBJ) $(DIF) -r $(IPATH)

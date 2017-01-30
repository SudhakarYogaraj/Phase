#include <iostream>

#include "Input.h"
#include "Communicator.h"
#include "CommandLine.h"
#include "ConstructGrid.h"
#include "PisoMultiphase.h"
#include "Viewer.h"
#include "RunControl.h"

int main(int argc, char* argv[])
{
    using namespace std;

    Communicator::init(argc, argv);

    Input input;
    Communicator comm;
    CommandLine(argc, argv);

    input.parseInputFile();

    shared_ptr<FiniteVolumeGrid2D> gridPtr(constructGrid(input));
    PisoMultiphase solver(input, comm, *gridPtr);

    Viewer viewer(input, solver);
    RunControl runControl;

    runControl.run(input, comm, solver, viewer);

    Communicator::finalize();

    return 0;
}


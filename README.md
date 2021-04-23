DCC Env
=======
This is an environment for use with [Veins-Gym](https://www2.tkn.tu-berlin.de/software/veins-gym/).
The scenario uses DCC, the aim is to learn optimal parameters.
For this, the observations are the average channel busy time, reward is a metric derived from the vehicles' age of information, and the action are the CBR values used by DCC for its transition.

Initial steps:
- install dependencies of the simulation: [SUMO](https://www.eclipse.org/sumo/) and [OMNeT++](https://omnetpp.org/) (such that you can run [Veins](http://veins.car2x.org/tutorial/))
- install the dependencies listed in `requirements.txt`
- build the simulation: `snakemake -jall`
- and run the example: `agents/trivial.py`.

For a deeper look into the simulation, see its configuration (`scenario`), and the `GymConnection` class.

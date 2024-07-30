import pathlib

#from wfcommons.wfchef.recipes import BlastRecipe
#from wfcommons.wfchef.recipes import BwaRecipe
#from wfcommons.wfchef.recipes import CyclesRecipe
#from wfcommons.wfchef.recipes import EpigenomicsRecipe
#from wfcommons.wfchef.recipes import GenomeRecipe
from wfcommons.wfchef.recipes import MontageRecipe

from wfcommons import WorkflowGenerator


import argparse

parser = argparse.ArgumentParser()
parser.add_argument("-s", "--size", type=int)

args = parser.parse_args()
size1 = args.size

print(size1)

#mylist = {200,1000, 2000, 4000, 8000, 10000, 15000, 18000, 20000, 25000, 30000}
mylist = {15000, 25000, 30000}
for x in mylist:
	print(x)
	generator = WorkflowGenerator(MontageRecipe.from_num_tasks(num_tasks=x))

	workflow = generator.build_workflow()

	workflow.write_dot(pathlib.Path(f'montage-workflow-'+str(x)+'.dot'))

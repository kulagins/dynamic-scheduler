import pathlib

from wfcommons.wfchef.recipes import BlastRecipe

from wfcommons import WorkflowGenerator


import argparse

parser = argparse.ArgumentParser()
parser.add_argument("-s", "--size", type=int)

args = parser.parse_args()
size1 = args.size

print(size1)

generator = WorkflowGenerator(BlastRecipe.from_num_tasks(num_tasks=size1))

workflow = generator.build_workflow()

workflow.write_dot(pathlib.Path(f'blast-workflow.dot'))

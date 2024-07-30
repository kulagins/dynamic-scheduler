import pathlib
import os
from wfcommons.common import Workflow

print(os.getcwd())
workflow = Workflow("atacseq")
workflow.read_dot(pathlib.Path('./../../input/atacseq_sparse.dot'))

# generate a few instances for statistics purposes
for i in range(0, 3):
    workflow.write_json(pathlib.Path(f'./dot-instances/atacseq-from-dot-{i}.json'))

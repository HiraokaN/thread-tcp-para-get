import csv
import math
import pandas as pd
import sys

argv_num = sys.argv
if len(argv_num) != 2:
  print("How to use : python static.py FILE_NAME")
  sys.exit()

df = pd.read_csv(sys.argv[1], index_col=0)

mean_bl_order = df.describe().at['mean', 'block-order']
mean_goodput = df.describe().at['mean', 'goodput']
stderr_bl_order = df.describe().at['std', 'block-order']/math.sqrt(df.describe().at['count', 'block-order'])
stderr_goodput = df.describe().at['std', 'goodput']/math.sqrt(df.describe().at['count', 'goodput'])

print("mean_bl_order="'{:.6f}'.format(mean_bl_order),"stderr_bl_order="'{:.6f}'.format(stderr_bl_order), sep=',')
print("mean_goodput="'{:.6f}'.format(mean_goodput),"stderr_goodput="'{:.6f}'.format(stderr_goodput), sep=',')
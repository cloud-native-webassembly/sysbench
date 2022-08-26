import sys
import os

sys.path.append(os.getcwd())
udf_home = os.environ.get("UDF_HOME")
if udf_home:
    sys.path.append(udf_home)

for syspath in sys.path:
    print(syspath)
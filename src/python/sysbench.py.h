unsigned char sysbench_py[] =
  "import sys\n"
  "import os\n"
  "\n"
  "sys.path.append(os.getcwd())\n"
  "udf_home = os.environ.get(\"UDF_HOME\")\n"
  "if udf_home:\n"
  "    sys.path.append(udf_home)\n"
  "\n"
  "for syspath in sys.path:\n"
  "    print(syspath)\n";
size_t sysbench_py_len = sizeof(sysbench_py) - 1;

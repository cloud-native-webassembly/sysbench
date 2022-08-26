export UDF_HOME=${PWD}/udf
./src/sysbench --data=${PWD}/UDF-Benchmark/string_to_bigint/bigint_1000.csv --type=python string_to_bigint run
./src/sysbench --socket=on --data=${PWD}/UDF-Benchmark/string_to_bigint/bigint_1000.csv --type=python string_to_bigint run

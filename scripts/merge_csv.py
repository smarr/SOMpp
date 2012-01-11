import os
import sys

#! /usr/bin/python
base_path = os.path.abspath(os.path.dirname(sys.argv[0]) + "/../")

#
# merges columns from several csv files to one csv file
#   the first column is always kept
#
def merge(columns, files):
    #first read all csv files to an 3-dim array raw-csv[file][line][column]
    raw_csv = []
    for f in files:
        csv_arr=[]
        for line in file(f).readlines():
            csv_arr.append([elem.strip() for elem in line.split(",")])
        raw_csv.append(csv_arr)

    #now merge
    res = []
    #generate new header
    header = ["name"]
    for i in xrange(len(files)):
        for col in columns:
            f_name = files[i]
            f_name = f_name[f_name.rfind("/")+1:f_name.find(".")]
            header.append(f_name + "." + raw_csv[i][0][col])
    res.append(", ".join(header))

    #now data
    noLines = len(raw_csv[0])
    for line in xrange(1,noLines):
        line_elems = [raw_csv[0][line][0]] #label of first column
        for csv in xrange(len(files)):
            for col in columns:
                line_elems.append(raw_csv[csv][line][col])
        res.append(", ".join(line_elems))
    return "\n".join(res)


if __name__ == "__main__":
    print merge((1,3),
            [   base_path+"/benchmark_results/generational_nocache_noTagging_som.csv",
                base_path+"/benchmark_results/generational_nocache_tagging_som.csv",
                base_path+"/benchmark_results/copying_nocache_noTagging_som.csv",
                base_path+"/benchmark_results/copying_nocache_tagging_som.csv",
                base_path+"/benchmark_results/mark_sweep_nocache_noTagging_som.csv",
                base_path+"/benchmark_results/mark_sweep_nocache_tagging_som.csv"
                ])


#!/home/hdp-guanggao/muyixiang/python2.7/bin/python

import os
import sys
from tempfile import NamedTemporaryFile
from streaming.runner import StreamingJob
from pipeline.dateset import get_datelist
from streaming.file_system import HadoopFS

script_dir = '.'
lib_dir = '/home/hdp-guanggao/muyixiang/python2.7/lib/python2.7/site-packages/'
util_egg = os.path.join(lib_dir, 'ctr_util-0.0.1-py2.7.egg')

input_dir = []
output_dir = sys.argv[2]

dirlist = os.popen('hadoop fs -ls ' + sys.argv[1], 'r' )
for line in dirlist:
    fields = line.strip().split(" "); 
    input_dir.append(fields[-1]); 

job = StreamingJob(
    script=os.path.join(script_dir, 'word_pair_click_info.py'),
    reducer_num=100,
    compress=True,
    files=['word_pair_info.txt'
    ],
    cmdenv={
    },
    eggs=[
        util_egg,
    ],
    jobconf={
      'mapred.job.priority':'HIGH'
      },
    input=input_dir,
    user='linjianguo',
    output=output_dir,
    job_name='word_pair_click_iinfo',

)
job.run()




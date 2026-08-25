compiling on windows : "gcc gds2nets.c gds_utils.c gds_fsms.c -o a.out -lws2_32"

[k for k, segs in enumerate([j.segments for j in complete_nets]) if any([i.shape[0] == [34565,147615] for i in segs if type(i)==Shape])]
[j for j in complete_nets[692].segments if j.touches([i for i in complete_nets[692].segments if (type(i)==Shape) and (i.shape[0] == [34565,147615])][0])]
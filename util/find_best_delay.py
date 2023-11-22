import sys

def datapoint_after(time_series:list, timestamp:int):

    for t,v in time_series:
        if t>timestamp:
            return (t,v)



def get_delay_from_time_series(time_series:list):

    last_timestamp, final_weight = time_series[-1]
    cuttoff_timestamp = last_timestamp-5000
    slope_calc_start_timestamp = cuttoff_timestamp-3000

    cutoff_datapoint = datapoint_after(time_series,cuttoff_timestamp)
    slope_calc_start_datapoint = datapoint_after(time_series,slope_calc_start_timestamp)

    slope = (cutoff_datapoint[1]-slope_calc_start_datapoint[1])/(cutoff_datapoint[0]-slope_calc_start_datapoint[0])

    b = cutoff_datapoint[1]-(cutoff_datapoint[0]*slope)

    calculated_delay = ((final_weight-b)/slope)-cuttoff_timestamp
    print(f"{slope}x+{b} -> {calculated_delay}")



def read_timeseries_from_file(f:str) -> list:



    ts = list()
    last_timestamp = 99999999999999
    with open(f) as data_file:
        for line in data_file.readlines():
            stripped = line.rstrip().split(";")
            timestamp, value= (int(stripped[0]),float(stripped[1]))
            if timestamp<last_timestamp:
                current_ts = list()
                ts.append(current_ts)
            last_timestamp=timestamp
            current_ts.append((timestamp,value))

    return ts

ts = read_timeseries_from_file("util/data")


for t in ts:
    get_delay_from_time_series(t)


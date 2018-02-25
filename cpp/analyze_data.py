import sys

# return the avarages for all test
def perform_avarage(arr,n):
    avgs = []
    for test_type_data in arr:
        avg = sum(test_type_data)/n
        avgs.append(avg)
    return avgs


#return the standard deviations for all test
def perform_standard_d(arr,n,avarages):
    if n <=1:
        return []
    deviation_arr = []
    i = 0
    for data in arr:
        sums = sum((val-avarages[i])**2 for val in data)
        standard = (sums/(n-1))**0.5
        deviation_arr.append(standard)
        i = i + 1
    return deviation_arr


def analyze(number_of_iters,test_numbers):
    # read file input
    fd = open("data.tempdata","r")
    file_data = fd.read()
    fd.close()
    # parse input as array of numbers
    tokened_data = file_data.replace('\n',':').split(":")
    tokened_data = a = [x for x in tokened_data if x != '']
    is_data = False
    float_arr = []
    # every second item is not float so take only floats
    for item in tokened_data:
        if is_data:
            value = float(item) # convert string to float
            float_arr.append(value)
        is_data = not is_data

    #init 2d array with floats for each type of test
    arr = []
    for i in range(0,test_numbers):
        v = []
        for j in range(0, number_of_iters):
            v.append(float_arr[i*number_of_iters + j])
        arr.append(v)

    # actual data extractions
    avarages = perform_avarage(arr,number_of_iters)
    std = perform_standard_d(arr,number_of_iters,avarages)
    print("avarage:", avarages)
    print("standard deviation:", std)
    return 0

analyze(int(sys.argv[1]),int(sys.argv[2])-1) # the bash script passes total number of script params which we need to remove 1

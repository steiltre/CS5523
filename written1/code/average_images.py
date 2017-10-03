import csv

def average_images(fin, fout):
    in_csv = open(fin, 'rb')
    out_csv = open(fout, 'wb')
    reader = csv.reader(in_csv)
    writer = csv.writer(out_csv, delimiter=",")
    for img in reader:
        # Average grayscale values in 4x4 chunks one row at a time
        out_img = [0] * 49
        out_img[1] = 16*int( img[1] )
        for row in range(1,7):
            for col in range(1,7):
                for avgd_row in range(1,4):
                    for avgd_col in range(1,4):
                        out_img[(row-1)*7 + col+1] += int( img[(row-1)*28*4 + (col-1)*4 + (avgd_row-1)*4 + avgd_col + 1] )
        out_img = [x / 16 for x in out_img]
        writer.writerow(out_img)

    in_csv.close()
    out_csv.close()


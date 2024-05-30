import pandas as pd
from matplotlib import pyplot as plt
import numpy as np
import seaborn as sns
import sys, os

import matplotlib.pyplot as plt
import matplotlib as mpl
from matplotlib.ticker import FormatStrFormatter
import argparse

def generate_senders_csv(path, n_senders, file_list):
    path = path
    num_senders = n_senders
    sender_num = 0

    df_sent_cols = [
        "Timestamp",
        "Flow ID",
        "Packet ID",
        "Packet Size",
        "IP ID",
        "DSCP",
        "ECN",
        "TTL",
        "Payload Size",
        "Proto",
        "Source IP",
        "Destination IP",
        "TCP Source Port",
        "TCP Destination Port",
        "TCP Sequence Number",
        "TCP Window Size",
        "Delay",
        "Workload ID",
        "Application ID",
        "Message ID",
    ]

    df_sent_cols_to_drop = [
        0,
        2,
        4,
        6,
        8,
        10,
        12,
        14,
        16,
        18,
        20,
        22,
        24,
        26,
        28,
        30,
        32,
        34,
        36,
        38,
        40,
    ]

    temp_cols = [
        "Timestamp",
        "Flow ID",
        "Packet ID",
        "Packet Size",
        "IP ID",
        "DSCP",
        "ECN",
        "TTL",
        "Payload Size",
        "Proto",
        "Source IP",
        "Destination IP",
        "TCP Source Port",
        "TCP Destination Port",
        "TCP Sequence Number",
        "TCP Window Size",
        "Delay",
        "Workload ID",
        "Application ID",
        "Message ID",
    ]

    temp = pd.DataFrame(columns=temp_cols)
    print(temp.head())

    files = file_list

    for file in files:

        sender_tx_df = pd.read_csv(path + file)
        sender_tx_df = pd.DataFrame(np.vstack([sender_tx_df.columns, sender_tx_df]))
        sender_tx_df.drop(
            sender_tx_df.columns[df_sent_cols_to_drop], axis=1, inplace=True
        )

        sender_tx_df.columns = df_sent_cols
        sender_tx_df["Packet ID"].iloc[0] = 0
        sender_tx_df["Flow ID"].iloc[0] = sender_tx_df["Flow ID"].iloc[1]
        sender_tx_df["IP ID"].iloc[0] = 0
        sender_tx_df["DSCP"].iloc[0] = 0
        sender_tx_df["ECN"].iloc[0] = 0
        sender_tx_df["TCP Sequence Number"].iloc[0] = 0
        # sender_tx_df["TTL"] = sender_tx_df.apply(lambda row: extract_TTL(row['Extra']), axis = 1)
        # sender_tx_df["Proto"] = sender_tx_df.apply(lambda row: extract_protocol(row['Extra']), axis = 1)
        sender_tx_df["Flow ID"] = [sender_num for i in range(sender_tx_df.shape[0])]
        sender_tx_df["Message ID"].iloc[0] = sender_tx_df["Message ID"].iloc[1]

        df_sent_cols_new = [
            "Timestamp",
            "Flow ID",
            "Packet ID",
            "Packet Size",
            "IP ID",
            "DSCP",
            "ECN",
            "Payload Size",
            "TTL",
            "Proto",
            "Source IP",
            "Destination IP",
            "TCP Source Port",
            "TCP Destination Port",
            "TCP Sequence Number",
            "TCP Window Size",
            "Delay",
            "Workload ID",
            "Application ID",
            "Message ID",
        ]
        sender_tx_df = sender_tx_df[df_sent_cols_new]

        # sender_tx_df.drop(['Extra'],axis = 1, inplace=True)
        temp = pd.concat([temp, sender_tx_df], ignore_index=True, copy=False)
        # sender_tx_df.drop(['Extra'],axis = 1, inplace=True)
        save_name = file.split(".")[0] + "_final.csv"
        sender_tx_df.to_csv(path + save_name, index=False)

    # temp.drop(['Extra'],axis = 1, inplace=True)
    print(temp.head())
    print(temp.columns)
    print(temp.shape)

    return temp

def plot_queue(path,queueframe,queuesize=100):

    values = [2, 3]
    dict_switches = {
        2: "A",
        3: "B"
    }

    for value in values:
        bottleneck_source = "/NodeList/{}/DeviceList/0/$ns3::CsmaNetDevice/TxQueue/PacketsInQueue".format(value)
        bottleneck_queue = queueframe[queueframe["source"] == bottleneck_source]
        print(bottleneck_source)

        plt.figure(figsize=(5,5))
        scs = sns.relplot(
            data=bottleneck_queue,
            kind='line',
            x='time',
            y='size',
            legend=False,
        )

        scs.fig.suptitle('Bottleneck queue on switch {} '.format(dict_switches[value]))
        scs.fig.suptitle('Queue on bottleneck switch')
        scs.set(xlabel='Simulation Time (seconds)', ylabel='Queue Size (packets)')
        left = bottleneck_queue["time"].iloc[0]
        right = bottleneck_queue["time"].iloc[-1]
        plt.xlim(left, right)
        #plt.ylim([0,queuesize])
        
        save_name = path + "Queue_profile_on_switch_{}".format(dict_switches[value]) + ".pdf"
        scs.fig.tight_layout()
        plt.savefig(save_name) 

def configure_sender_csv(path, sender_list):

    df = pd.read_csv(path + sender_list[0])
    # Drop columns 1 and 3

    df.drop(df.columns[[0, 2, 4, 6]], axis=1, inplace=True)

    # Rename columns
    df.columns = [
        "Timestamp",
        "Packet Size",
        "Packet ID",
        "TCP Sequence Number",
        "Blank",
    ]

    # Drop the blank column
    df.drop(df.columns[[-1]], axis=1, inplace=True)

    # Save as final csv
    df.to_csv(path + sender_list[0].split(".")[0] + "_final.csv", index=False)

    return df

def timeseries_plot(path, df, start, stop, step=0.005, _bin=0.01):

    # Drop first row
    df = df.iloc[1:]

    e2ed = df["Delay"]
    # Get relative timestamp
    # df["Timestamp"] = df["Timestamp"] - df["Timestamp"].iloc[0]
    time = df["Timestamp"]

    # # Plot timeseries

    # left = df["Timestamp"].iloc[0]
    # right = df["Timestamp"].iloc[-1]

    # left = 2
    # right = 5

    # plt.figure(figsize=(10, 5))
    # plt.plot(time, e2ed, label="End-to-end delay")
    # plt.xlabel("Time (s)")
    # plt.ylabel("Delay (s)")
    # plt.title(f"End-to-end delay vs time")
    # plt.xlim(left, right)
    # plt.legend()
    # plt.savefig(path + f"plot_timeseries_range_{left}_{right}.pdf")

    df["Size"] = df["Packet Size"] * 8

    timestarts = np.arange(start, stop, step)

    rates = []
    for timestart in timestarts:
        timeend = timestart + _bin
        df_time = df[(df["Timestamp"] >= timestart) & (df["Timestamp"] <= timeend)]
        rates.append(df_time["Size"].sum() / _bin / 1e6)

    return time, e2ed, timestarts, rates

def plot_sending_rate(df, start, stop, step=0.005, _bin=0.01):

    # df["Timestamp"] = df["Timestamp"] - df["Timestamp"].iloc[0]
    df["Size"] = df["Packet Size"] * 8

    # Bin every 5ms of timestamps and count the number of packets sent in that time
    timestarts = np.arange(start, stop, step)
    # print(timestarts)
    # Get all timestamps from the df which are 0.01 seconds from each time start
    counts = []
    sizes = []
    times = []
    rates = []
    for timestart in timestarts:
        #print("Timestart:", timestart)
        timeend = timestart + _bin
        #print("Timeend:", timeend)
        df_time = df[(df["Timestamp"] >= timestart) & (df["Timestamp"] <= timeend)]
        #print("Number of packets:", len(df_time))
        #print("Total size:", df_time["Packet Size"].sum())
        counts.append(len(df_time))
        sizes.append(df_time["Packet Size"].sum())
        rates.append(df_time["Size"].sum() / _bin / 1e6)

    # # Plot subplot
    # plt.figure(figsize=(10, 5))

    # plt.subplot(1, 2, 1)
    # plt.plot(timestarts, counts, label="Number of packets sent")
    # plt.xlabel("Time (s)")
    # plt.ylabel("Number of packets")
    # plt.title(f"Number of packets sent vs time")
    # plt.legend()

    # plt.subplot(1, 2, 2)
    # plt.plot(timestarts, sizes, label="Total size of packets sent")
    # plt.xlabel("Time (s)")
    # plt.ylabel("Total size of packets (bytes)")
    # plt.title(f"Total size of packets sent vs time")
    # plt.legend()
    # plt.tight_layout()
    # plt.savefig(f"results_test/plot_sending_rate_start_{start}_stop_{stop}.pdf")
    return timestarts, counts, sizes, rates

def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--path", type=str, default="results_test_large/")
    parser.add_argument("--apprate", type=int, required=True)
    parser.add_argument("--n_senders", type=int, required=True)
    parser.add_argument("--rst", type=bool, default=False)
    parser.add_argument("--infinitedata", type=bool, default=False)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--tcpcc", type=str, default="cubic")
    parser.add_argument("--bw", type=int, default=5)
    
    return parser.parse_args()

def main():

    args = get_args()
    # Get path
    base_path = args.path

    
    if args.apprate == 1 and args.n_senders == 6:
        print("1 Mbps base rate for applications, 6 senders flows")
        if args.infinitedata:
            path = base_path + f"1Mbps_6senders_inf_rst_cubic_{args.seed}_{args.bw}mbps/"
        elif args.rst:
            path = base_path + f"1Mbps_6senders_rst_{args.seed}/"
        else:
            path = base_path + "1Mbps_6senders/"
        if not os.path.isdir(path):
            os.mkdir(path)

    if args.apprate == 1 and args.n_senders == 9:
        print("1 Mbps base rate for applications, 9 senders flows")
        if args.infinitedata:
            path = base_path + f"1Mbps_9senders_inf_rst_cubic_{args.seed}_{args.bw}mbps/"
        elif args.rst:
            path = base_path + f"1Mbps_9senders_rst_{args.seed}/"
        else:
            path = base_path + "1Mbps_9senders/"
        if not os.path.isdir(path):
            os.mkdir(path)

    # path = "results_test/"
    file_list = [
        "switch_A_port1.csv",
        "switch_A_port2.csv",
        "switch_C_port1.csv",
        "switch_C_port2.csv",
        "switch_C_port3.csv",
        "switch_E_port1.csv",
        "switch_E_port2.csv",
    ]

    # Check if file 
    for count, file in enumerate(file_list):
        file_name = file.split(".")[0] + "_final.csv"
        file = os.path.join(path, file_name)
        if not os.path.isfile(file):
            print("File does not exist")
            df = generate_senders_csv(path, 1, file_list)
            print("Generated CSV")

        else:
            print("File exists")
            df = pd.read_csv(file)

        print(df.head())
    
    
        start = 1
        stop = 30
        step = 0.005
        _bin = 1

        times, e2ed, timestarts, throughputs = timeseries_plot(path, df, start, stop, step=step, _bin=_bin)

        left = 1
        right = 30
        # top = 150
        # bottom = 0
        # Plot 4 subplots

        plt.figure(figsize=(10, 5))

        plt.subplot(1, 2, 1)
        # Make e2ed in ms
        e2ed = e2ed * 1000
        plt.plot(times, e2ed, label="End-to-end delay")
        plt.xlabel("Time (s)")
        plt.ylabel("Delay (ms)")
        plt.title(f"End-to-end delay vs time")
        plt.xlim(left, right)
        #plt.ylim(0.03, 0.035)
        plt.legend(fontsize=6, loc='upper right')
        plt.tight_layout()

        # Plot throughput
        plt.subplot(1, 2, 2)
        plt.plot(timestarts, throughputs, label="Throughput", color="red")
        plt.xlabel("Time (s)")
        plt.ylabel("Throughput (Mbps)")
        plt.title(f"Throughput vs time")
        plt.xlim(left, right)
        plt.legend(fontsize=6, loc='upper right')
        plt.tight_layout()
        plt.savefig(path + f"plot_timeseries_start_{left}_stop_{right}_{count}.pdf")

    

    # Rename queue file
    if not os.path.isfile(path+"queue.csv"):
        os.rename(path+f"small_test_no_disturbance_with_message_ids{args.seed}_queues.csv", path+"queue.csv")
    # Plot queue
    queueframe = pd.read_csv(path + "queue.csv", names=["source", "time", "size"])
    # plot_queue(path, queueframe, queuesize=100)

    # Rename drop file
    if not os.path.isfile(path+"drops.csv"):
        os.rename(path+f"small_test_no_disturbance_with_message_ids{args.seed}_drops.csv", path+"drops.csv")
    dropframe = pd.read_csv(path+"drops.csv",
                             names=["source", "time", "packetsize"])


    print("Drop fraction:", len(dropframe) / (len(dropframe) + len(df)))

    # Plot queue size at receiver
    path = path

    values = [6, 8, 10]
    dict_switches = {
        6: "A",
        8: "C",
        10: "E",
    }

    for value in values:
        bottleneck_source = "/NodeList/{}/DeviceList/0/$ns3::CsmaNetDevice/TxQueue/PacketsInQueue".format(value)
        bottleneck_queue = queueframe[queueframe["source"] == bottleneck_source]
        print(bottleneck_source)

        plt.figure(figsize=(5,5))
        scs = sns.relplot(
            data=bottleneck_queue,
            kind='line',
            x='time',
            y='size',
            legend=False,
            errorbar=None,
        )

        scs.fig.suptitle(f'Bottleneck queue on switch {dict_switches[value]} '.format())
        scs.fig.suptitle('Queue on bottleneck switch')
        scs.set(xlabel='Simulation Time (seconds)', ylabel='Queue Size (packets)')
        plt.xlim([0,30])

        if value == 6:
            plt.ylim([0,100])
        else:
            plt.ylim([0,100])
        
        save_name = path + f"Queue_profile_on_switch_{dict_switches[value]}" + ".pdf"
        scs.fig.tight_layout()
        plt.savefig(save_name) 

    # Plot the scatter plot of packet drops
    plt.figure(figsize=(5,5))
    # One dimensional scatter plot
    plt.scatter(dropframe["time"], dropframe["packetsize"], s=1, label="Packet drops")
    plt.xlabel("Time (s)")
    plt.ylabel("Packet Size (bytes)")
    plt.title("Packet drops")
    plt.savefig(path + "packet_drops.pdf")



if __name__ == "__main__":
    main()




import argparse
import math

NUM_THREAD = 128

def number_of_buckets(group_size):
    return math.ceil(1.5 * (group_size - 1))

def replies_per_thread(num_users, num_bucket):
    return max(1, math.ceil((num_users * num_bucket) / NUM_THREAD))

def main():
    # Define and parse cli flags
    parser = argparse.ArgumentParser(
        prog="Pirates Benchmarking Tool",
        description="Run Pirates benchmarks"
    )
    parser.add_argument('-g', '--groupsize', type=int, help='Group size')
    parser.add_argument('-u', '--users', type=int, help='Number of users')
    args = parser.parse_args()

    print(f'Replies per thread: {replies_per_thread(args.users, number_of_buckets(args.groupsize))}')

if __name__ == "__main__":
    main()


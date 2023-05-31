import subprocess
import argparse
import random
import string

GROUP_MIN        = 2
GROUP_MAX        = 8
GROUP_STEP       = 1
GROUP_DEFAULT    = 3

#USER_EXP_MIN     = 4
#USER_EXP_MAX     = 9
#USER_EXP_STEP    = 1
#USER_EXP_DEFAULT = 6

USER_MIN     = 3
USER_MAX     = 12
USER_STEP    = 1
USER_DEFAULT = 8

SNIPPET_MIN      = 40
SNIPPET_MAX      = 320
SNIPPET_STEP     = 20
SNIPPET_DEFAULT  = 60

def clear_logs():
    with open('./logs/caller.csv', 'w') as file:
        header = 'id,group_size,num_users,snippet_size,t_b_caller,t_a_encryption,t_a_encoding\n'
        file.write(header)

    with open('./logs/relay.csv', 'w') as file:
        header = 'id,t_b_relay,t_a_relay\n'
        file.write(header)

    with open('./logs/worker.csv', 'w') as file:
        header = 'id,t_b_worker,t_a_preprocessing,t_a_replies\n'
        file.write(header)

    with open('./logs/callee.csv', 'w') as file:
        header = 'id,t_b_callee,t_a_pir,t_a_decryption,t_a_decoding\n'
        file.write(header)

def gen_env(group_size, num_users, snippet_size):
    # generate random 8 letter run id
    run_id = ''.join(random.choice(string.ascii_lowercase) for i in range(8))

    body = (
        'SUBNET=172.31.0.0/16\n'
        'IP_WORKER=172.31.0.2\n'
        'IP_CALLER=172.31.0.3\n'
        'IP_CALLEE=172.31.0.4\n'
        'IP_RELAY=172.31.0.5\n'
        'NUM_ROUNDS=20\n'
        f'SNIPPET_SIZE={snippet_size}\n'
        f'GROUP_SIZE={group_size}\n'
        f'NUM_USERS={num_users}\n'
        f'RUN_ID={run_id}\n'
    )
    with open('.env', 'w') as file:
        file.write(body)

def start_containers():
    cmd = ['docker', 'compose', 'up', '--build']
    subprocess.run(cmd)

def run_group_bench():
    for g in range(GROUP_MIN, GROUP_MAX, GROUP_STEP):
        for s in range(SNIPPET_MIN, SNIPPET_MAX, SNIPPET_STEP):
            gen_env(g, USER_DEFAULT, s)
            start_containers()

def run_user_bench():
    for u in range(USER_MIN, USER_MAX, USER_STEP):
        for s in range(SNIPPET_MIN, SNIPPET_MAX, SNIPPET_STEP):
            gen_env(GROUP_DEFAULT, u, s)
            start_containers()

def main():
    # Define and parse cli flags
    parser = argparse.ArgumentParser(
        prog="Pirates Benchmarking Tool",
        description="Run Pirates benchmarks"
    )
    parser.add_argument('-g', '--groupsize', action='store_true', help='Run group size benchmarks')
    parser.add_argument('-u', '--users', action='store_true', help='Run number of users benchmarks')
    parser.add_argument('-c', '--clear', action='store_true', help='Clear log files')
    args = parser.parse_args()

    if args.clear:
        clear_logs()

    if args.groupsize:
        print('Running group size benchmark')
        run_group_bench()

    if args.users:
        print('Running number of users benchmark')
        run_user_bench()

if __name__ == "__main__":
    main()


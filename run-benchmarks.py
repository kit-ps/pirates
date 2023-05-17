import subprocess
import argparse

GROUP_MIN        = 2
GROUP_MAX        = 22
GROUP_STEP       = 2
GROUP_DEFAULT    = 8

USER_EXP_MIN     = 4
USER_EXP_MAX     = 14
USER_EXP_STEP    = 1
USER_EXP_DEFAULT = 10

SNIPPET_MIN      = 40
SNIPPET_MAX      = 320
SNIPPET_STEP     = 20
SNIPPET_DEFAULT  = 120

def gen_env(group_size, num_users, snippet_size):
    body = (
        'SUBNET=172.123.0.0/16\n'
        'IP_WORKER=172.123.0.2\n'
        'IP_CALLER=172.123.0.3\n'
        'IP_CALLEE=172.123.0.4\n'
        'IP_RELAY=172.123.0.5\n'
        'NUM_MESSAGE=512\n'
        f'MESSAGE_SIZE={snippet_size}\n'
        'NUM_ROUNDS=1\n'
        f'GROUP_SIZE={group_size}\n'
        f'NUM_USERS={num_users}\n'
    )
    with open('.env', 'w') as file:
        file.write(body)

def start_containers():
    cmd = ['sudo', 'docker', 'compose', 'up', '--build']
    subprocess.run(cmd)

def run_group_bench():
    for g in range(GROUP_MIN, GROUP_MAX, GROUP_STEP):
        gen_env(g, 2**USER_EXP_DEFAULT, SNIPPET_DEFAULT)
        start_containers()

def run_user_bench():
    for u in range(USER_EXP_MIN, USER_EXP_MAX, USER_EXP_STEP):
        gen_env(GROUP_DEFAULT, 2**u, SNIPPET_DEFAULT)
        start_containers()

def run_snippet_bench():
    for s in range(SNIPPET_MIN, SNIPPET_MAX, SNIPPET_STEP):
        gen_env(GROUP_DEFAULT, 2**USER_EXP_DEFAULT, s)
        start_containers()

def main():
    # Define and parse cli flags
    parser = argparse.ArgumentParser(
        prog="Pirates Benchmarking Tool",
        description="Run Pirates benchmarks"
    )
    parser.add_argument('-g', '--groupsize', action='store_true', help='Run group size benchmarks')
    parser.add_argument('-s', '--snippetpsize', action='store_true', help='Run snippet size benchmarks')
    parser.add_argument('-u', '--users', action='store_true', help='Run number of users benchmarks')
    args = parser.parse_args()

    if (args.groupsize):
        print('Running group size benchmark')
        run_group_bench()

    if (args.groupsize):
        print('Running number of users benchmark')
        run_user_bench()

    if (args.groupsize):
        print('Running snippet size benchmark')
        run_snippet_bench()

if __name__ == "__main__":
    main()

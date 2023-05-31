import pandas
import datetime
import sys

WARMUP_ROUNDS = 4

pandas.set_option('display.max_rows', None)
roles = ['caller', 'relay', 'worker', 'callee']

# Read the csv files into separate dataframes
raw_dfs = []
for role in roles:
    df = pandas.read_csv(f'./logs/{role}.csv')
    raw_dfs.append(df)

# Join the dataframes at run id
df = raw_dfs[0]
for idx, f in enumerate(raw_dfs[1:]):
    df = pandas.merge(df, f, on='id')

df['mouth_to_ear'] = (df['t_a_decoding'] - df['t_b_caller'] + df['snippet_size'] + 10000) / 1000
df['pir_reply_time'] = (df['t_a_replies'] - df['t_b_worker']) / 1000 
df['ratio'] = df['pir_reply_time'] / df['snippet_size']
df['exp_id'] = df['id'].str.split('-').apply(lambda x: x[0])
df['run_id'] = df['id'].str.split('-').apply(lambda x: x[1])
print(df[['exp_id', 'group_size', 'num_users', 'snippet_size', 'mouth_to_ear']])

result_dict = {
    'group_size': [],
    'num_users': [],
    'snippet_size': [],
    'mean_m2e': [],
    'std_dev_m2e': [],
    'mean_ratio': [],
    }
for experiment in list(dict.fromkeys(df['exp_id'].tolist())):
    tmp_df = df[df['exp_id'] == experiment]
    result_dict['group_size'].append(tmp_df['group_size'].iat[0])
    result_dict['num_users'].append(tmp_df['num_users'].iat[0])
    result_dict['snippet_size'].append(tmp_df['snippet_size'].iat[0])
    result_dict['mean_m2e'].append(tmp_df['mouth_to_ear'].iloc[WARMUP_ROUNDS:].mean())
    result_dict['std_dev_m2e'].append(tmp_df['mouth_to_ear'].iloc[WARMUP_ROUNDS:].std())
    result_dict['mean_ratio'].append(tmp_df['ratio'].iloc[WARMUP_ROUNDS:].mean())

result_df = pandas.DataFrame(result_dict)

print(result_df)

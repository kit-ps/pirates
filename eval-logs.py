import pandas
import datetime
import sys

WARMUP_ROUNDS = 10

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

df['mouth_to_ear'] = (df['t_a_decoding'] - df['t_b_caller'] + df['snippet_size'] + 10000 + 15000) / 1000
df['pir_reply_time'] = (df['t_a_replies'] - df['t_b_worker']) / 1000 
df['ratio'] = df['pir_reply_time'] / df['snippet_size']
df['exp_id'] = df['id'].str.split('-').apply(lambda x: x[0])
df['run_id'] = df['id'].str.split('-').apply(lambda x: x[1])

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
result_df['params'] = result_df['group_size'].map(str) + result_df['num_users'].map(str)


filtered_df = pandas.DataFrame(columns=result_df.columns)
for i, param in enumerate(list(dict.fromkeys(result_df['params'].tolist()))):
    tmp_df = result_df[result_df['mean_ratio'].astype(float) < 1.1] 
    tmp_df = tmp_df[tmp_df['params'] == param] 
    min_row = tmp_df[tmp_df['mean_m2e'] == tmp_df['mean_m2e'].min()]
    filtered_df = pandas.concat([filtered_df, min_row])

print(filtered_df)
#result_df.to_csv('./logs/gs_0737.csv')

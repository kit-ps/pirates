import pandas
import datetime
import sys

WARMUP_ROUNDS = 10

pandas.set_option('display.max_rows', None)
roles = ['caller', 'relay', 'worker', 'callee']

def print_step_times(dataframe):
    print(f"Voice Encode: {round((dataframe['t_a_encoding'] - dataframe['t_b_caller']).mean() / 1000, 2)}ms")
    print(f"Encryption: {round((dataframe['t_a_encryption'] - dataframe['t_a_encoding']).mean() / 1000, 2)}ms")
    print(f"Caller->Relay: {round((dataframe['t_b_relay'] - dataframe['t_a_encryption']).mean() / 1000, 2)}ms")
    print(f"Relay->Worker: {round((dataframe['t_b_worker'] - dataframe['t_b_relay']).mean() / 1000, 2)}ms")
    print(f"DB Preproc.: {round((dataframe['t_a_preprocessing'] - dataframe['t_b_worker']).mean() / 1000, 2)}ms")
    print(f"PIR Replies: {round((dataframe['t_a_replies'] - dataframe['t_a_preprocessing']).mean() / 1000, 2)}ms")
    print(f"Worker->Callee: {round((dataframe['t_b_callee'] - dataframe['t_a_replies']).mean() / 1000, 2)}ms")
    print(f"PIR Decode: {round((dataframe['t_a_pir'] - dataframe['t_b_callee']).mean() / 1000, 2)}ms")
    print(f"Decrypt: {round((dataframe['t_a_decryption'] - dataframe['t_a_pir']).mean() / 1000, 2)}ms")
    print(f"Voice Decode: {round((dataframe['t_a_decoding'] - dataframe['t_a_decryption']).mean() / 1000, 2)}ms")
    print(f"Mouth-to-ear: {round(dataframe['mouth_to_ear'].mean(), 2)}ms")

# Read the csv files into separate dataframes
raw_dfs = []
for role in roles:
    df = pandas.read_csv(f'./logs/{role}.csv')
    raw_dfs.append(df)

# Join the dataframes at run id
df = raw_dfs[0]
for idx, f in enumerate(raw_dfs[1:]):
    df = pandas.merge(df, f, on='id')

df['mouth_to_ear'] = (df['t_a_decoding'] - df['t_b_caller'] + df['snippet_size']*1000 + 10000 + 15000) / 1000
df['blocking_time'] = (df['t_a_replies'] - df['t_b_worker']) / 1000 
df['ratio'] = df['blocking_time'] / df['snippet_size']
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
    tmp_df = result_df.loc[
        (result_df['mean_ratio'].astype(float) < 1.1) &
        (result_df['params'] == param)
    ]
    min_row = tmp_df[tmp_df['mean_m2e'] == tmp_df['mean_m2e'].min()]
    filtered_df = pandas.concat([filtered_df, min_row])

print(filtered_df)

print("\nLarge Snippet Breakdown")
print("-----------------------")
large_snippet_df = df.loc[
    (df['group_size'].astype(int) == 2) &
    (df['num_users'].astype(int) == 6) &
    (df['snippet_size'].astype(int) == 200)
]
print_step_times(large_snippet_df)
 
print("\nLarge Group Breakdown")
print("---------------------")
large_group_df = df.loc[
    (df['group_size'].astype(int) == 5) &
    (df['num_users'].astype(int) == 6) &
    (df['snippet_size'].astype(int) == 80)
]
print_step_times(large_group_df)

print("\nMany Users Breakdown")
print("--------------------")
many_users_df = df.loc[
    (df['group_size'].astype(int) == 3) &
    (df['num_users'].astype(int) == 11) &
    (df['snippet_size'].astype(int) == 80)
]
print_step_times(many_users_df)

import pandas
import datetime
import sys

WARMUP_ROUNDS = 4

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

#for experiment in set(df['exp_id'].to_list()):
for experiment in list(dict.fromkeys(df['exp_id'].tolist())):
    tmp_df = df[df['exp_id'] == experiment]
    print(f"GS: {tmp_df['group_size'].iat[0]}, NU: {tmp_df['num_users'].iat[0]}, SS: {tmp_df['snippet_size'].iat[0]}")
    print(f"Mean: {tmp_df['mouth_to_ear'].iloc[WARMUP_ROUNDS:].mean()}")
    print(f"Std. Dev.: {tmp_df['mouth_to_ear'].iloc[WARMUP_ROUNDS:].std()}\n")

#print(df['continuous'].isin([True]))

#tmp_df = df[df['continuous'] == True]
#print(tmp_df['group_size', 'num_users', 'snippet_size','mouth_to_ear','continuous'])
#df.to_csv("./logs/combined.csv")
#print(tmp_df[['exp_id', 'group_size', 'num_users', 'snippet_size', 'mouth_to_ear', 'continuous']])

tmp_df = df[df['run_id'] == '5']
print(tmp_df[['snippet_size', 'pir_reply_time', 'ratio', 'mouth_to_ear']])
    

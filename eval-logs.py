import pandas
import datetime
import sys

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
print(df)
#df['mouth-to-ear'] = (df['t_a_decoding'] - df['t_b_caller'] + df['snippet_size'] + 10) / 1000
#print(df[['group_size', 'num_users', 'snippet_size', 'mouth-to-ear']])

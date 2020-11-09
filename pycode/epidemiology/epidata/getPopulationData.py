import os
import sys
import pandas
import numpy as np
from collections import namedtuple
from epidemiology.epidata  import getDataIntoPandasDataFrame as gd
from epidemiology.epidata import defaultDict as dd


def get_population_data(read_data=dd.defaultDict['read_data'],
                       out_form=dd.defaultDict['out_form'],
                       out_folder=dd.defaultDict['out_folder']):

   print("Warning: getpopulationdata is not working correctly. A bug workaround has been applied.")

   Data = namedtuple("Data", "filename item columns_wanted filename_out")

   d1 = Data("FullDataB", '5dc2fc92850241c3be3d704aa0945d9c_2', ["LAN_ew_RS", 'LAN_ew_GEN','LAN_ew_EWZ'], "PopulStates")
   d2 = Data("FullDataL", 'b2e6d8854d9744ca88144d30bef06a76_1', ['RS', 'GEN','EWZ'], "PopulCounties")

   #d = [d1, d2]
   d = [d1]

   directory = os.path.join(out_folder, 'Germany/')
   gd.check_dir(directory)

   for i in range(len(d)):
     get_one_data_set(read_data, out_form, directory, d[i])

def get_one_data_set(read_data, out_form, directory, d):

   if(read_data):
     # if once dowloaded just read json file
     file = os.path.join(directory, d.filename + ".json")

     try:
        df = pandas.read_json(file)

     except ValueError:
        exit_string = "Error: The file: " + file + " does not exist. Call program without -r flag to get it."
        sys.exit(exit_string)
   else:

     # Supported data formats:
     load = { 
        'csv': gd.loadCsv,
        'geojson': gd.loadGeojson
      }

     # Get data:
     df = load['csv'](d.item)

     # output data to not always download it
     gd.write_dataframe(df, directory, d.filename, "json")

   print("Available columns:", df.columns)

   # Filter data for Bundesland/Landkreis and Einwohnerzahl (EWZ)
   dfo = df[d.columns_wanted]
   dfo = dfo.rename(columns=dd.GerEng)
   gd.write_dataframe(dfo, directory, d.filename_out, out_form)

# creates 7 new counties that were formed since 2011 and deletes old counties
def get_new_counties(data_temp):
   # create 7 new counties
   data_temp = np.append(data_temp, np.zeros((7,data_temp.shape[1])), axis=0)

   # Göttingen
   data_temp[-7,0] = 3159

   # Mecklenburgische Seenplatte
   data_temp[-6,0] = 13071

   # Landkreis Rostock
   data_temp[-5,0] = 13072

   # Vorpommern Rügen
   data_temp[-4,0] = 13073

   # Nordwestmecklenburg
   data_temp[-3,0] = 13074

   # Vorpommern Greifswald
   data_temp[-2,0] = 13075

   # Ludwigslust-Parchim
   data_temp[-1,0] = 13076


   to_delete = []
   for i in range(len(data_temp[:,0])):
      # fuse "Göttingen" and "Osterode am Harz" into Göttingen
      if data_temp[i,0] in [3152, 3156]:
         data_temp[-7, 1:] += data_temp[i,1:]
         to_delete.append(i)

      # fuse "Müritz", "Neubrandenburg", "Mecklenburg-Sterlitz" 
      # and "Demmin" into "Mecklenburgische Seenplatte"
      if data_temp[i,0] in [13056, 13002, 13055, 13052]:
         data_temp[-6, 1:] += data_temp[i,1:]
         to_delete.append(i)

      # fuse "Bad Doberan and Güstrow" into "Landkreis Rostosck"
      if data_temp[i,0] in [13051, 13053]:
         data_temp[-5, 1:] += data_temp[i,1:]
         to_delete.append(i)

      # fuse "Rügen", "Stralsund" and "Nordvorpommern" into 
      # "Vorpommern Rügen"
      if data_temp[i,0] in [13061, 13005, 13057]:
         data_temp[-4, 1:] += data_temp[i,1:]
         to_delete.append(i)

      # fuse "Wismar" and "Nordwestmecklenburg" into 
      # "Nordwestmecklenburg"
      if data_temp[i,0] in [13006, 13058]:
         data_temp[-3, 1:] += data_temp[i,1:]
         to_delete.append(i)

      # fuse "Ostvorpommern", "Uecker-Randow" and "Greifswald"
      # into "Vorpommern Greifswald"
      if data_temp[i,0] in [13059, 13062, 13001]:
         data_temp[-2, 1:] += data_temp[i,1:]
         to_delete.append(i)

      # fuse "Ludwigslust" adn "Parchim" into "Ludwigslust-Parchim"
      if data_temp[i,0] in [13054, 13060]:
         data_temp[-1, 1:] += data_temp[i,1:]
         to_delete.append(i)

   data_temp = np.delete(data_temp, to_delete, 0)
   sorted_inds = np.argsort(data_temp[:,0])
   data_temp = data_temp[sorted_inds, :]
   
   return data_temp

def get_age_population_data(read_data=dd.defaultDict['read_data'],
                       out_form=dd.defaultDict['out_form'],
                       out_folder=dd.defaultDict['out_folder']):

   path_counties = 'http://hpcagainstcorona.sc.bs.dlr.de/data/migration/'
   path_reg_key = 'https://www.zensus2011.de/SharedDocs/Downloads/DE/Pressemitteilung/DemografischeGrunddaten/1A_EinwohnerzahlGeschlecht.xls?__blob=publicationFile&v=5'
   path_zensus = 'https://opendata.arcgis.com/datasets/abad92e8eead46a4b0d252ee9438eb53_1.csv'
   
   #read tables
   counties = pandas.read_excel(os.path.join(path_counties,'kreise_deu.xlsx'),sheet_name=1, header=3)
   reg_key = pandas.read_excel(path_reg_key, sheet_name='Tabelle_1A', header=12)
   zensus = pandas.read_csv(path_zensus)
   
   
   #find region keys for census population data
   key = np.zeros((len(zensus)))
   for i in range(len(key)):
      for j in range(len(reg_key)):
         if zensus.Name.values[i] == reg_key['NAME'].values.astype(str)[j]:
            if zensus.EWZ.values[i] == round(reg_key['Zensus_EWZ'].values[j]*1000):
               key[i] = reg_key['AGS'].values[j]
   
   unique, inds, count = np.unique(key, return_index=True, return_counts=True)
   
   male = ['M_Unter_3', 'M_3_bis_5', 'M_6_bis_14', 'M_15_bis_17', 'M_18_bis_24',
           'M_25_bis_29', 'M_30_bis_39', 'M_40_bis_49', 'M_50_bis_64',
           'M_65_bis_74', 'M_75_und_aelter']
   female = ['W_Unter_3', 'W_3_bis_5', 'W_6_bis_14', 'W_15_bis_17', 'W_18_bis_24',
           'W_25_bis_29', 'W_30_bis_39', 'W_40_bis_49', 'W_50_bis_64',
           'W_65_bis_74', 'W_75_und_aelter']
   columns = ['Key', 'Total', '<3 years', '3-5 years', '6-14 years', '15-17 years', '18-24 years',
              '25-29 years', '30-39 years', '40-49 years', '50-64 years',
              '65-74 years', '>74 years']
   
   # add male and female population data
   data = np.zeros((len(inds), len(male)+2))
   data[:,0] = key[inds].astype(int)
   data[:,1] = zensus['EWZ'].values[inds].astype(int)
   for i in range(len(male)):
      data[:, i+2] = zensus[male[i]].values[inds].astype(int) + zensus[female[i]].values[inds].astype(int)
  
   data = get_new_counties(data)
   
   # compute ratio of current and 2011 population data
   ratio = np.ones(len(data[:,0]))
   for i in range(len(ratio)):
      for j in range(len(counties)-11):
          if not counties['Schlüssel-nummer'].isnull().values[j]:
              if data[i,0] == int(counties['Schlüssel-nummer'].values[j]):
                   ratio[i] = counties['Bevölkerung2)'].values[j]/data[i, 1]


   # adjust population data for all ages to current level
   data_current = np.zeros(data.shape)
   data_current[:, 0] = data[:, 0].copy()
   for i in range(len(data[0, :]) - 1):
      data_current[:, i + 1] = np.multiply(data[:, i + 1], ratio)
   
   
   #create dataframe
   df = pandas.DataFrame(data.astype(int), columns=columns)
   df_current = pandas.DataFrame(np.round(data_current).astype(int), columns=columns)
   
   
   
   directory = out_folder  
   directory = os.path.join(directory, 'Germany/')
   gd.check_dir(directory)
   
   
   gd.write_dataframe(df, directory, 'county_population', out_form)
   gd.write_dataframe(df_current, directory, 'county_current_population', out_form)

def main():

   [read_data, out_form, out_folder] = gd.cli("population")
   get_age_population_data(read_data, out_form, out_folder)


if __name__ == "__main__":

   main()

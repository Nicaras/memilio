#############################################################################
# Copyright (C) 2020-2021 German Aerospace Center (DLR-SC)
#
# Authors: Kathrin Rack
#
# Contact: Martin J. Kuehn <Martin.Kuehn@DLR.de>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#############################################################################
"""
@file getDataIntoPandasDataFrame.py
@brief Tools to convert data to pandas dataframes

This tool contains
- load geojson format
- load csv format
- organizes the command line interface
- check if directory exists and if not creates it
- writes pandas dataframe to file of three different formats
"""

import os
from urllib.request import urlopen
import json
import argparse
import datetime
import requests
import pandas as pd
from io import BytesIO, StringIO
from zipfile import ZipFile

from memilio.epidata import defaultDict as dd
from memilio import progress_indicator


def download_file(url, encoding=None, chunk_size=1024, timeout=None, progress_function=None):
    """! Download a file using GET over HTTP.

    @param url Full url of the file to download.
    @param encoding If speciefied, used to decode the downloaded bytes.
    @param chunk_size Number of Bytes downloaded at once. Only used when a
        progress_function is specified. For a good display of progress, this
        size should be about the speed of your internet connection in Bytes/s.
        Can be set to None to let the server decide the chunk size (may be
        equal to the file size).
    @param timeout Timeout in seconds for the GET request.
    @param progress_function Function called regularly, with the current
        download progress in [0,1] as a float argument.
    @return File as BytesIO by default, or as StringIO if encoding was set.
    """
    # send GET request as stream so 
    req = requests.get(url, stream=True, timeout=timeout)
    if req.status_code != 200: #e.g. 404
        raise requests.exceptions.HTTPError("HTTPError: "+str(req.status_code))
    # get file size from http header
    # this is only the number of bytes downloaded, the size of the actual file
    # may be larger (e.g. when 'content-encoding' is gzip; decoding is handled
    # by iter_content)
    file_size = int(req.headers.get('content-length'))
    file = bytearray() # file to be downloaded
    if progress_function:
        progress = 0
        # download file as bytes via iter_content
        for chunk in req.iter_content(chunk_size=chunk_size):
            file += chunk # append chunk to file
            # note: len(chunk) may be larger (e.g. encoding)
            # or smaller (for the last chunk) than chunk_size
            progress = min(progress+chunk_size, file_size)
            progress_function(progress/file_size)
    else: # download without tracking progress
        for chunk in req.iter_content(chunk_size=None):
            file += chunk # append chunk to file
    # return the downloaded content as file like object
    if encoding:
        return StringIO(str(file, encoding=encoding))
    else:
        return BytesIO(file)


def loadGeojson(
        targetFileName, apiUrl='https://opendata.arcgis.com/datasets/',
        extension='geojson'):
    """! Loads data default: ArcGIS data sets in GeoJSON format. (pandas DataFrame)
    This routine loads datasets default: ArcGIS data sets in GeoJSON format of the given public
    data item ID into a pandas DataFrame and returns the DataFrame. Trivial
    information gets removed by JSON normalization and dropping of always
    constant data fields.

    @param targetFileName -- File name without ending, for ArcGIS use public data item ID (string)
    @param apiUrl -- URL to file (default: ArcGIS data sets API URL (string, default
              'https://opendata.arcgis.com/datasets/'))
    @param extension -- Data format extension (string, default 'geojson')
    return dataframe
    """
    url = apiUrl + targetFileName + '.' + extension

    try:
        with urlopen(url) as res:
            data = json.loads(res.read().decode())
    except OSError as err:
        raise FileNotFoundError(
            "ERROR: URL " + url + " could not be opened.") from err

    # Shape data:
    df = pd.json_normalize(data, 'features')

    # Make-up (removing trivial information):
    df.drop(columns=['type', 'geometry'], inplace=True)
    df.rename(columns=lambda s: s[11:], inplace=True)

    return df


def loadCsv(
        targetFileName, apiUrl='https://opendata.arcgis.com/datasets/',
        extension='.csv', param_dict={}, encoding=None):
    """! Loads data sets in CSV format. (pandas DataFrame)
    This routine loads data sets (default from ArcGIS) in CSV format of the given public data
    item ID into a pandas DataFrame and returns the DataFrame.

    @param targetFileName -- file name which should be downloaded, for ArcGIS it should be public data item ID (string)
    @param apiUrl -- API URL (string, default
              'https://opendata.arcgis.com/datasets/')
    @param extension -- Data format extension (string, default '.csv')
    @param param_dict -- Defines the parameter for read_csv:
            "sep": Delimiter to use (string, default ',')
            "header": Row to use for column labels (Use None if there is no header) (int, default 0)
            "encoding": Encoding to use for UTF when reading (string, default None)
            "dtype": Data type for data or column (dict of column -> type, default None)
    @return dataframe 
    """

    url = apiUrl + targetFileName + extension
    param_dict_default = {"sep": ',', "header": 0, "encoding": encoding, 'dtype': None}

    for k in param_dict_default:
        if k not in param_dict:
            param_dict[k] = param_dict_default[k]

    try:
        try: # to download file from url and show download progress
            with progress_indicator.Percentage(message="Downloading " + url) as p:
                file = download_file(url, 1024, param_dict["encoding"], None, p.set_progress)
            df = pd.read_csv(file, **param_dict)
        except requests.exceptions.RequestException:
            df = pd.read_csv(url, **param_dict)
    except OSError as err:
        raise FileNotFoundError(
            "ERROR: URL " + url + " could not be opened.") from err

    return df


def loadExcel(targetFileName, apiUrl='https://opendata.arcgis.com/datasets/',
              extension='.xls', param_dict={}):
    """ Loads ArcGIS data sets in Excel formats (xls,xlsx,xlsm,xlsb,odf,ods,odt). (pandas DataFrame)

    This routine loads data sets (default from ArcGIS) in Excel format of the given public data
    item ID into a pandas DataFrame and returns the DataFrame.

    Keyword arguments:
    targetFileName -- file name which should be downloaded, for ArcGIS it should be public data item ID (string)
    apiUrl -- API URL (string, default
              'https://opendata.arcgis.com/datasets/')
    extension -- Data format extension (string, default '.xls')
    param_dict -- Defines the parameter for read_excel:
            "sheetname": sheet from Excel file which should be in DataFrame
            (string (sheetname) or integer (zero-indexed sheet position), default 0)
            "header": row to use for column labels (Use None if there is no header) (int, default 0)
            "engine": defines engine for reading (str, default 'openpyxl')
    """
    url = apiUrl + targetFileName + extension
    param_dict_default = {"sheet_name": 0, "header": 0, "engine": 'openpyxl'}

    for k in param_dict_default:
        if k not in param_dict:
            param_dict[k] = param_dict_default[k]

    try:
        if extension == '.zip':
            file_compressed = ZipFile(
                BytesIO(urlopen(url).read())).filelist[0].filename
            df = pd.read_excel(
                ZipFile(BytesIO(urlopen(url).read())).open(file_compressed),
                **param_dict)
        else:
            df = pd.read_excel(url, **param_dict)
    except OSError as err:
        raise FileNotFoundError(
            "ERROR: URL " + url + " could not be opened.") from err

    return df


def cli(what):
    """! Defines command line interface

    The function parameter "what" is used as a dictionary key.
    The return of the dictionary is either a list of a string and a list of keywords.
    The string is the message that should be printed when working on the specific package.
    The further list, contains all special command line arguments which are needed for this package.

    If the key is not part of the dictionary the program is stopped.

    The following default arguments are added to the parser:
    - read-from-disk
    - file-format, choices = ['json', 'hdf5', 'json_timeasstring']
    - out_path
    - no_raw
    The default values are defined in default dict.

    Depending on what following parser can be added:
    - start_date
    - end_date
    - impute_dates
    - moving_average
    - make_plot
    - split_berlin
    - rep_date
    - sanitize_data

    @param what Defines what packages calls and thus what kind of command line arguments should be defined.
    """

    # TODO: may it would be easier to make a dict like the following one together with a function to get key:
    # TODO: all should automatically do everything
    # cli_dict2 = {"end_date": ['divi'],
    #                "plot": ['cases'],
    #                "start_date": ['divi']                 }

    cli_dict = {"divi": ['Downloads data from DIVI', 'start_date', 'end_date', 'impute_dates', 'moving_average', 'make_plot'],
                "cases": ['Download case data from RKI', 'start_date', 'end_date', 'impute_dates', 'moving_average', 'make_plot', 'split_berlin', 'rep_date'],
                "cases_est": ['Download case data from RKI and JHU and estimate recovered and deaths', 'start_date', 'end_date', 'impute_dates', 'moving_average', 'make_plot', 'split_berlin', 'rep_date'],
                "population": ['Download population data from official sources'],
                "commuter_official": ['Download commuter data from official sources', 'make_plot'],
                "vaccination": ['Download vaccination data', 'start_date', 'end_date', 'impute_dates', 'moving_average', 'make_plot', 'sanitize_data'],
                "testing": ['Download testing data', 'start_date', 'end_date', 'impute_dates', 'moving_average', 'make_plot'],
                "jh": ['Downloads data from Johns Hopkins University', 'start_date', 'end_date', 'impute_dates', 'moving_average', 'make_plot'],
                "hospitalization": ['Download hospitalization data', 'start_date', 'end_date', 'impute_dates', 'moving_average', 'make_plot'],
                "sim": ['Download all data needed for simulations', 'start_date', 'end_date', 'impute_dates', 'moving_average', 'make_plot', 'split_berlin', 'rep_date', 'sanitize_data']}

    try:
        what_list = cli_dict[what]
    except KeyError:
        raise ValueError("Wrong key or cli_dict.")

    out_path_default = dd.defaultDict['out_folder']

    check_dir(out_path_default)

    parser = argparse.ArgumentParser(description=what_list[0])
    group = parser.add_mutually_exclusive_group()

    group.add_argument(
        '-r', '--read-data',
        help='Reads the data from file "json" instead of downloading it.',
        action='store_true')

    parser.add_argument(
        '-f', '--file-format', type=str, default=dd.defaultDict
        ['file_format'],
        choices=['json', 'hdf5', 'json_timeasstring'],
        help='Defines output format for data files. Default is \"' +
        str(dd.defaultDict['file_format'] + "\"."))
    parser.add_argument('-o', '--out-folder', type=str,
                        default=out_path_default,
                        help='Defines folder for output.')
    parser.add_argument(
        '-n', '--no-raw', default=dd.defaultDict['no_raw'],
        help='Defines if raw data will be stored for further use.',
        action='store_true')

    if 'start_date' in what_list:
        parser.add_argument(
            '-s', '--start-date',
            help='Defines start date for data download. Should have form: YYYY-mm-dd.'
            'Default is 2020-04-24',
            type=lambda s: datetime.datetime.strptime(s, '%Y-%m-%d').date(),
            default=dd.defaultDict['start_date'])
    if 'end_date' in what_list:
        parser.add_argument(
            '-e', '--end-date',
            help='Defines date after which data download is stopped.'
            'Should have form: YYYY-mm-dd. Default is today',
            type=lambda s: datetime.datetime.strptime(s, '%Y-%m-%d').date(),
            default=dd.defaultDict['end_date'])
    if 'impute_dates' in what_list:
        parser.add_argument(
            '-i', '--impute-dates',
            help='the resulting dfs contain all dates instead of'
            ' omitting dates where no data was reported', action='store_true')
    if 'moving_average' in what_list:
        parser.add_argument(
            '-m', '--moving-average', type=int, default=0,
            help='Compute a moving average of N days over the time series')
    if 'make_plot' in what_list:
        parser.add_argument('-p', '--make-plot', help='Plots the data.',
                            action='store_true')
    if 'split_berlin' in what_list:
        parser.add_argument(
            '-b', '--split-berlin',
            help='Berlin data is split into different counties,'
            ' instead of having only one county for Berlin.',
            action='store_true')
    if 'rep_date' in what_list:
        parser.add_argument(
            '--rep-date', default=False,
            help='If reporting date is activated, the reporting date'
            'will be prefered over possibly given dates of disease onset.',
            action='store_true')
    if 'sanitize_data' in what_list:
        parser.add_argument(
            '-sd', '--sanitize_data', type=int, default=1,
            help='Redistributes cases of every county either based on regions ratios or on thresholds and population'
        )
    args = parser.parse_args()

    return vars(args)


def append_filename(filename, impute_dates, moving_average):
    """! Creates consistent file names for all output.
    """
    if moving_average > 0:
        filename = filename + '_ma' + str(moving_average)
    elif impute_dates:
        filename = filename + '_all_dates'

    return filename


def check_dir(directory):
    """! Checks existence and creates folder

    It is checked if the folder given in the parameter "directory" exists.
    If it does not exist it is created.

    @param directory directory which should exist
    """

    # check if directory exists or create it
    if not os.path.exists(directory):
        os.makedirs(directory)


def write_dataframe(df, directory, file_prefix, file_type, param_dict={}):
    """! Writes pandas dataframe to file

    This routine writes a pandas dataframe to a file in a given format.
    The filename is given without ending.
    A file_type can be
    - json
    - json_timeasstring [Default]
    - hdf5
    - txt
    The file_type defines the file format and thus also the file ending.
    The file format can be json, hdf5 or txt.
    For this option the column Date is converted from datetime to string.

    @param df pandas dataframe (pandas DataFrame)
    @param directory directory where to safe (string)
    @param file_prefix filename without ending (string)
    @param file_type defines ending (string)
    @param param_dict defines parameters for to_csv/txt(dictionary)

    """

    outForm = {'json': [".json", {"orient": "records"}],
               'json_timeasstring': [".json", {"orient": "records"}],
               'hdf5': [".h5", {"key": "data"}],
               'txt': [".txt", param_dict]}

    try:
        outFormEnd = outForm[file_type][0]
        outFormSpec = outForm[file_type][1]
    except KeyError:
        raise ValueError(
            "Error: The file format: " + file_type +
            " does not exist. Use json, json_timeasstring, hdf5 or txt.")

    out_path = os.path.join(directory, file_prefix + outFormEnd)

    if file_type == "json":
        df.to_json(out_path, **outFormSpec)
    elif file_type == "json_timeasstring":
        if dd.EngEng['date'] in df.columns:
            if not isinstance(df.Date.values[0], type("string")):
                df.Date = df.Date.dt.strftime('%Y-%m-%d')
        df.to_json(out_path, **outFormSpec)
    elif file_type == "hdf5":
        df.to_hdf(out_path, **outFormSpec)
    elif file_type == "txt":
        df.to_csv(out_path, **outFormSpec)

    print("Information: Data has been written to", out_path)


class DataError(Exception):
    """ Error for handling incomplete or unexpected Data """
    pass

import os
import shutil
import sqlite3

folder = '../images_video'
for the_file in os.listdir(folder):
    file_path = os.path.join(folder, the_file)
    try:
        if os.path.isfile(file_path):
            os.unlink(file_path)
        #elif os.path.isdir(file_path): shutil.rmtree(file_path)
    except Exception, e:
        print e

sqlite_file = '../db/VizLensDynamic.db'
table_name = 'images_video'

# Connecting to the database file
conn = sqlite3.connect(sqlite_file)
c = conn.cursor()

# C) Updates the newly inserted or pre-existing entry            
c.execute("DELETE FROM {tn}".format(tn=table_name))

conn.commit()
conn.close()
# test_framework_visma
A testing framework made for Visma Solutions Oy as a pre-interview task

## Requirements

### Compile requirements
 * make
 * glib2 (libglib2.0-dev)
 * glib json (libjson-glib-dev)
 * Curl (libcurl4-openssl-dev / libcurl4-gnutls-dev)
	
### Runtime requirements
 * glib2 (libglib-2.0-0)
 * glib json (libjson-glib-1.0-0)
 * Curl (curl)

## Compiling
 * compile: make
 * debug: make debug

## Running

### To get CLI UI
./testfw -u (username)

or with default username:

make run

or

./testfw

### To do a single run as specific user and test
./testfw -u (username) -t (testname)

Returns 0 when user and test was found. 1 is returned otherwise


### To log the results and send them via email

####PRE-Requirements:

*mailx* is installed on the system

*testfw.conf* is configured

*run_test_with_mail.sh* has execute bit set (chmod +x)

#### RUNNING

''./run_test_with_mail.sh USERNAME TESTNAME''

This will run ./testfw with both parameters, log results to file named "run_log_USERNAME_DATE" in the same folder and sends the log to USERNAME (also in case of error) using variables for server and server defined in *testfw.conf*.


## Approach
This testing framework is written with C using glib, glib-json and curl libraries. The basic idea is to use the REST API defined in preferences.json for testing with the given testcases. Contains basic CLI UI and possibility to do a single run with username and testname searched from tests/username/preferences.json.

This framework is highly configurable. Multiple different json test files are supported. A main file contains the generic test details, such as REST API URL and the files to be used as testing or adding new data. Each file has multiple parameters to configure from REST API path to HTTP method. Each of the testfiles or added data jsons can have two extra types for member field values ({parent} and {getinfo}). If a member field has either of this value it has to include also a configuration json for this member field (see Structure of testcase files). This configuration tells from which file response the value is to be retrived with a specific member field name. Also, any field containing integers can be configured and they are treated as double type.

Binary was created to support also a standalone run on Linux and with a script (run_test_with_mail.sh) enables logging of the results using username and current time for log file name and then the result file can be sent via email to user (as username = email) if correct binary (mailx) is installed and settings are configured (testfw.conf).

### Order of things 

First the logged in user is used as a path to open the preferences.json in folder "tests/<username>". File preferences.json details all the tests of this user which are listed to cli ui. From this ui the user can select which test to conduct. At this point the separate test files are not loaded. Program can be also run by specifying both username and testname, this will load the user's preferences and requested test. The procedure in both cases is the same. 

Next after selecting the test it will be run. Following sequence is used:
 - Build the sequence in which the tests are run. The test file with id "login" is always first and next are the testfiles in ascending order starting from id "0" which is the case creation file.
 - Start by loading a testfile in sequence and go through them in ascending order
 	- Go through each test file in database if method is for sending (POST/PUT) and search for {parent} and {getinfo} fields, include them in the testfile structure for future use of getting files details what to do with these member fields.
 	- Next replace any {id} strings in "path" of the testfile with the guid of the case.
 	- Conduct test:
 		- if id is "login" send the specified credentials file and add token for http functions. 
 		- If id is "0" create the case by sending the specified data and verify its values. 
 		- For rest of the tests do as the files specify; replace any {parent} and {getinfo} member values according to their configurations if method to send is POST/PUT. This will create a hash table of values to be replaced (both {parent} and {getinfo}) in a single run creating a new JSON for sending. Last, send the altered JSON to server and verify the values in response.
 	- Unload all tests in reverse order. If the testfile has "delete" member defined as "no" in preferences.json do nothing for it. Get the correct guids from responses and send DELETE to REST API URL using the appropriate path for this testfile.
 	- Return to cli UI and ask for further info from user (r - retry, u - return to user selection, m - return to main (testlist) and q - quit).

### Structure of testcase files

For each user a seprate folder with the users full name (e.g. john.doe@severa.com) has to be created, this must contain a preferences file such as [preferences.json](https://github.com/HateBreed/test_framework_visma/blob/master/tests/john.doe%40severa.com/preferences.json). In this file each test belongin to the user are listed under "tests" containg test name, url, server encoding, array containing fields that are numerical and array of files. 

Current folder structure is:
 - tests/
 	- username1/
 		- preferences.json
 		- testname1/
 			- login.json
 			- test1.json
 			- test2.json
 		- testname2/
 			- login.json
 			- Test.json
 	- username2
 		- preferences.json
 		- testX
 			- login.json
 			- X.json


In preferences.json:
* name - testname
* URL - REST API URL
* encoding - character encoding of the server

The member field integerfields can be used to list all the member fields that are to be treated as integers (double). There is no need to have any values for each, the member names are used to form a list of these fields.

Each file entry in preferences.json must contain following:
 * id - File identification used within test framework databases. ID "login" is the credentials json and others must have identification as integer starting from "0". No limit restriction.
 * file - The actual file located under "testname" folder defined in preferences.json
 * path - REST API path that is added to the url defined in preferences.json. Can have an {id} in path which will be replaced with the case id, therefore, any file with id higher than "1" can include this.
 * method - HTTP method to use when sending this data, the file defined by "file" field is sent only when this field is POST
 * delete - Must contain either "yes" or "no" telling the framework whether this file requires that data is deleted afterwards. If "yes" then the test framework will send DELETE to corresponding REST API using both path and identification returned by the server.
 
Each json file that is going to be sent can contain any data but anything defined within "data" member field must be inside an array. These files will be sent "as is" but two values for json members can be used for getting information from elsewhere:
 * {parent} - Tells that a response to parent file must be checked. This needs to be specified in a separate json file named as <this.json>.info.<membername>.json (e.g. Expense.json with member field "case_guid" results in Expense.json.info.case_guid.json). This file must contain following fields within ["data" member](https://github.com/HateBreed/test_framework_visma/blob/master/tests/john.doe%40severa.com/test1/Expense.json.info.case_guid.json) :
   * root_task - "yes" or "no", if "yes" then the root_task entry within a json object will be used to get the value withheld by "search_member"
   * search_member - Name of the member field to be searched within "search_file". Value of this member field is assigned to the member field to which this file is connected to
   * search_file - Id (in preferences.json) of the file which contains the "search_member"
 * {getinfo} - Defines that more information is required from server. The file named as <this.json>.getinfo.<membername>.json (e.g., [Expense.json.getinfo.product_guid.json](https://github.com/HateBreed/test_framework_visma/blob/master/tests/john.doe%40severa.com/test1/Expense.json.getinfo.product_guid.json) details from which path and with what method more information regarding this member is to be retrieved from REST API. The fields that the file must contain are:
   * path - the path to be added to testcase REST API url
   * method - HTTP method to use for accessing REST API url 
 


## Challenges:

Biggest challenge was in learning the glib-json api and getting to know the REST API. I have to admit, the specification in email was somewhat confusing but with a little amount of investigation and rock hard determination correct values were eventually figured out for different fields.

Other minor challenges included the C coding in general, there might still be some flaws in the code. Therefore, I would conduct a thorough revision of this if there would be time for it. 

## Improvement ideas / future development:

I had in mind to build a server-client architecture for this framework using SCTP for communication and it will remain a question whether I have time to implement that. This would include listing existing tests, adding new with file upload. The new test would be added to preferences.json and files stored on server for future use. 

The server-client would also enable using of timed tests with logging information sent to the email of the user. Timing of tasks would be easily done in Linux-environment on server side using cron. An entry would be added to cron to run a script, which would send a tailored system signal to the running server process to do a test of the user who requested timing of test. It would include sending the username and testname from the script. Sending of email could be done with curl, for instance, so no new libraries have to be included. The email would include either detailed information of the test or just a summary of potential errors. 

Or if the server is not running a single instance could be executed as it is shown earlier how to do a single run with cli parameters. This would require that a proper logging system is implemented that, depending on how the framework is run, prints log to screen and to log-file or only to log-file. **[UPDATE]**: Added a script that should be run with username (email address) and testname of that user, which saves the result of the test to a log file named "run_log_(username)_(date)" in the same folder and sends the result to username (email) if SMTP_SRV and SENDER_ADDRESS are set using *mailx* binary.

Last, there should be a way to add the tests instead of creating the folder structure manually. Now the only way is to create/manipulate preferences.json manually and add tests there, then to create test folder and move tests into that folder. 

## Compromises made

No compromises, except that the user interface is lacking finesse. It is crude. A client-server paradigm would ease this that any language could be then used build a client for doing the tests while this code serves as a server.

## Open issues / need to improve

Character encoding is not done at any point. This must be addressed as it results in € character to be printed as ? and messing up some printing to cli. Need to look at glib documentation how to do this correctly. **UPDATE**: the character encoding is easy with glib but it does not seem to work properly yet and, therefore, is disabled although there is a possibility to configure server side encoding.

There are memory leaks. Some might be because of agile coding and hurry, others may be false positives reported by valgrind because glib utilizes memory slicing not well understood by valgrind.

Error checking is lacking in many cases. Again, a symptom caused by agile coding and hurry.

<del>User interface is buggy.</del>

<del>Reading of the required fields ({parent}) and fields requiring more info ({getinfo}) is not efficient. This must be integrated into loading process of the JSON and not to do separately.</del> This was implemented.

<del>Replacing of JSON member field values is not efficient, it does the replacing one at a time and then rewrites the json by changing only one value. This must be changed to one JSON write function that accepts a GHashTable of member field names as keys and their new values so all can be replaced in one run.</del> This was implemented.

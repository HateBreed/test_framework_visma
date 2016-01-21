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
make

## Running
./testfw -u (username)


## Approach
This testing framework is written with C using glib, glib-json and curl libraries. The basic idea is to use the REST API defined in preferences.json for testing with the given testcases.

This framework is highly configurable. Multiple different json test files are supported. A main file contains the generic test details, such as REST API URL and the files to be used as testing or adding new data. Each file has multiple parameters to configure from REST API path to HTTP method. Each of the testfiles or added data jsons can have two extra types for member field values ({parent} and {getinfo}). If a member field has either of this value it has to include also a configuration json for this member field (see Structure of testcase files). This configuration tells from which file response the value is to be retrived with a specific member field name.

### Order of things 

First the logged in user is used as a path to open the preferences.json in folder "tests/<username>". File preferences.json details all the tests of this user which are listed to cli ui. From this ui the user can select which test to conduct. At this point the separate test files are not loaded. 

Next after selecting the test it will be run. Following sequence is used:
 - Go through each test file in database and search for {parent} and {getinfo} fields, include them in the testfile structure for future use of getting files details what to do with these member fields.
 - Build the sequence in which the tests are run. The test file with id "login" is always first and next are the testfiles in ascending order starting from id "0" which is the case creation file.
 - Run the tests according to the built sequence. 
   - Start by loading a testfile in sequence
   - Next replace any {id} strings in "path" of the testfile with the guid of the case.
   - Conduct test, if id is "login" add token for http functions. If id is "0" create the case and verify its values. For rest of the tests do as the files specify; replace any {parent} and {getinfo} member values according to their configurations, send json to server and verify the response.
 - Unload all tests in reverse order. If the testfile has "delete" member defined as "no" in preferences.json do nothing for it. Get the correct guids from responses and send DELETE to REST API URL using the appropriate path for this testfile.
 - Return to cli UI and ask for further info from user (r - retry, q - quit).

### Structure of testcase files

For each user a seprate folder with the users full name (e.g. john.doe@severa.com) has to be created, this must contain a preferences file such as [preferences.json](https://github.com/HateBreed/test_framework_visma/blob/master/tests/john.doe%40severa.com/preferences.json). In this file each test belongin to the user are listed under "tests" containg test name, url and array of files. 

Each file entry in preferences.json must contain following:
 * id - File identification used within test framework databases. ID "login" is the credentials json and others must have identification as integer starting from "0". No limit restriction.
 * file - The actual file located under "testname" folder defined in preferences.json
 * path - REST API path that is added to the url defined in preferences.json. Can have an {id} in path which will be replaced with the case id, therefore, any file with id higher than "1" can include this.
 * method - HTTP method to use when sending this data, the file defined by "file" field is sent only when this field is POST
 * delete - Must contain either "yes" or "no" telling the framework whether this file requires that data is deleted afterwards. If "yes" then the test framework will send DELETE to corresponding REST API using both path and identification returned by the server.
 
Each json file that is going to be sent can contain any data, these will be sent "as is" but two values for json members can be used for getting information from elsewhere:
 * {parent} - Tells that a response to parent file must be checked. This needs to be specified in a separate json file named as <this.json>.info.<membername>.json (e.g. Expense.json with member field "case_guid" results in Expense.json.info.case_guid.json). This file must contain following fields within ["data" member](https://github.com/HateBreed/test_framework_visma/blob/master/tests/john.doe%40severa.com/test1/Expense.json.info.case_guid.json) :
   * root_task - "yes" or "no", if "yes" then the root_task entry within a json object will be used to get the value withheld by "search_member"
   * search_member - Name of the member field to be searched within "search_file". Value of this member field is assigned to the member field to which this file is connected to
   * search_file - Id (in preferences.json) of the file which contains the "search_member"
 * {getinfo} - Defines that more information is required from server. The file named as <this.json>.getinfo.<membername>.json (e.g., Expense.json.getinfo.product_guid.json) details from which path and with what method more information regarding this member is to be retrieved from REST API. The fields that the file must contain are:
   * path - the path to be added to testcase REST API url
   * method - HTTP method to use for accessing REST API url 
 


## Challenges:

Biggest challenge was in learning the glib-json api and getting to know the REST API. I have to admit, the specification in email was somewhat confusing but with a little amount of investigation and rock hard determination correct values were eventually figured out for different fields.

Other minor challenges included the C coding in general, there might still be some flaws in the code. Therefore, I would conduct a thorough revision of this if there would be time for it. 

## Improvement ideas / future development:

I had in mind to build a server-client architecture for this framework using SCTP for communication and it will remain a question whether I have time to implement that. This would include listing existing tests, adding new with file upload. The new test would be added to preferences.json and files stored on server for future use. 

The server-client would also enable using of timed tests with logging information sent to the email of the user. Timing of tasks would be easily done in Linux-environment on server side using cron. An entry would be added to cron to run a script, which would send a tailored system signal to the running server process to do a test of the user who requested timing of test. It would include sending the username and testname from the script. Sending of email could be done with curl, for instance, so no new libraries have to be included. The email would include either detailed information of the test or just a summary of potential errors. Or if the server is not running there could be 

## Compromises made

No compromises, except that the user interface is lacking finesse. It is crude. A client-server paradigm would ease this that any language could be then used build a client for doing the tests while this code serves as a server.

## Open issues

There are memory leaks. Some might be because of agile coding and hurry, others may be false positives reported by valgrind because glib utilizes memory slicing not well understood by valgrind.

Error checking is lacking in many cases. Again, a symptom caused by agile coding and hurry.

User interface is buggy.

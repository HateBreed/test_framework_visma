#include "tests.h"

/*
	Select given test from user_preferences
	Create new parser and load each file separately
	Check fields of the file for {parent} unless 0
	Create JsonGenerator from parser root JsonNode
	use connectionutils to establish http connection with curl to the URL of test
	get data from JsonGenerator to be sent to server
	Get the response from connectionutils
	Check the same fields that were in the file
	If id == 0, store the json/required fields in a hashtable
	To get required fields while processing first all files:
		Get all member names having {parent}
		Add each of the member names into hashtable as keys with "" content
		When creating the case (with id 0) get each of these member names from
			the response and store them into the hashtable where these can be
			retrieved when required
			
	Last, delete all file contents that were sent (with DELETE method) and
	finally delete the case
*/
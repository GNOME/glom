#!/usr/bin/python
import create_db

try:
	create_db.test_create_db([
		'CreatePostgreSQLdatabaseinitsownfolder,tobehostedbythiscomputer.',
		'Createdatabaseinitsownfolder,tobehostedbythiscomputer.'
	])
except create_db.BackendUnavailableError:
	raise LdtpExecutionError('Self-hosted postgresql backend not available')

/*
 * Seal sub-command
 *
 * Copyright (c) 2016, Wind River Systems, Inc.
 * All rights reserved.
 *
 * See "LICENSE" for license terms.
 *
 * Author:
 *	  Lans Zhang <jia.zhang@windriver.com>
 */

#include <cryptfs_tpm2.h>

static char *opt_auth_password;
static bool opt_setup_key;
static bool opt_setup_passphrase;
static char *opt_passphrase;
static TPMI_ALG_HASH opt_pcr_bank_alg = TPM_ALG_NULL;

static void
show_usage(char *prog)
{
	info_cont("\nUsage: %s seal <object> <args>\n", prog);
	info_cont("\nobject:\n");
	info_cont("  The object to be sealed. The allowed values are:\n"
		  "    passphrase: Passphrase used to encrypt LUKS\n"
		  "    key: Primary key used to seal the passphrase\n"
		  "    all: All above\n");
	info_cont("\nargs:\n");
	info_cont("  --pcr-bank-alg, -P: (optional) Use the specified PCR "
		  "bank to bind the created primary key and passphrase.\n");
	info_cont("  --auth, -a: (optional) Set the authorization value for "
		  "owner hierarchy.\n");
	info_cont("  --passphrase, -p: (optional) Set the passphrase value.\n");
}

static int
parse_arg(int opt, char *optarg)
{
	switch (opt) {
	case 'a':
		if (strlen(optarg) > sizeof(TPMU_HA)) {
			err("The authorization value for owner hierarchy is "
			    "no more than %d characters\n",
			    (int)sizeof(TPMU_HA));
			return -1;
		}
		opt_auth_password = optarg;
                break;
	case 'p':
		opt_passphrase = optarg;
		break;
	case 'P':
		if (!strcasecmp(optarg, "sha1"))
			opt_pcr_bank_alg = TPM_ALG_SHA1;
		else if (!strcasecmp(optarg, "sha256"))
			opt_pcr_bank_alg = TPM_ALG_SHA256;
		else if (!strcasecmp(optarg, "sha384"))
			opt_pcr_bank_alg = TPM_ALG_SHA384;
		else if (!strcasecmp(optarg, "sha512"))
			opt_pcr_bank_alg = TPM_ALG_SHA512;
		else if (!strcasecmp(optarg, "sm3_256"))
			opt_pcr_bank_alg = TPM_ALG_SM3_256;
		else {
			err("Unrecognized PCR bank algorithm\n");
			return -1;
		}
		break;
	case 1:
		if (!strcasecmp(optarg, "key"))
			opt_setup_key = 1;
		else if (!strcasecmp(optarg, "passphrase"))
			opt_setup_passphrase = 1;
		else if (!strcasecmp(optarg, "all")) {
			opt_setup_key = 1;
			opt_setup_passphrase = 1;
		} else {
			err("Unrecognized value\n");
			return -1;
		}
                break;
	default:
		return -1;
	}

	if (opt_passphrase && !opt_setup_passphrase) {
		warn("-p option is ignored if the object to be sealed is not "
		     "passphrase\n");
		opt_passphrase = NULL;
	}

	return 0;
}

static int
run_seal(char *prog)
{
	int rc = 0;

	if (opt_setup_key) {
		rc = cryptfs_tpm2_create_primary_key(opt_pcr_bank_alg,
						     opt_auth_password);
		if (rc)
			return rc;
	}

	if (opt_setup_passphrase) {
		size_t size = opt_passphrase ? strlen(opt_passphrase) : 0;
		rc = cryptfs_tpm2_create_passphrase(opt_passphrase, size,
						    opt_pcr_bank_alg,
						    opt_auth_password);
		if (rc)
			return rc;
	}

	return rc;
}

static struct option long_opts[] = {
	{ "auth", required_argument, NULL, 'a' },
	{ "passphrase", required_argument, NULL, 'p' },
	{ "pcr-bank-alg", required_argument, NULL, 'P' },
	{ 0 },	/* NULL terminated */
};

subcommand_t subcommand_seal = {
	.name = "seal",
	.optstring = "-a:p:P:",
	.long_opts = long_opts,
	.parse_arg = parse_arg,
	.show_usage = show_usage,
	.run = run_seal,
};

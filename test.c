void envid2env(u_int envid, struct Env **penv,int checkperm) {
	struct Env *e;

	if(envid == 0)
	{
		*penv = curenv;
		return 0;
	}
	e = &envs[ENVX(envid)];)

	if(e->env_status == ENV_FREE || e->env_id != envid)
	{
		return -E_BAD_ENV;
	}


}

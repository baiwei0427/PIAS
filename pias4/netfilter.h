#ifndef NETFILTER_H
#define NETFILTER_H

/* Install Netfilter hooks */
bool PIAS_Netfilter_Init(void);
/* Uninstall Netfilter hooks */
void PIAS_Netfilter_Exit(void);

#endif

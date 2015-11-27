SUFFIDX = net_ring net_main net_bundle net_xml net_work net_hashmap net_forward

all :
	@for i in ${SUFFIDX}; do make -C $$i || exit -1; cp -af $$i/*.o ./build/; done
	@make -C build

clean distclean dep depend:
	@for i in ${SUFFIDX} build; do make -C $$i $@; done

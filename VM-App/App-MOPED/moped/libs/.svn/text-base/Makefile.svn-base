# Targets 
.PHONY: clean realclean all $(LIBNAME)
all : done

done :
	@echo [ Extract LIBS ]
	@tar -xzf libs.tgz
	@for f in `find * -maxdepth 0 -type d`; do cd $$f; make -s; cd ..;  done;
	@for f in `find * -maxdepth 0 -type d`; do cd $$f; make -s clean; cd ..;  done;
	@touch done

clean:
	@-$(RM) -rf `find * -maxdepth 0 -type d`
	@-$(RM) done

	

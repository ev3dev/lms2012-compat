/*
 * f_sourcesink.c - USB peripheral source/sink configuration driver
 *
 * Copyright (C) 2003-2008 David Brownell
 * Copyright (C) 2008 by Nokia Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/* #define VERBOSE_DEBUG */


/*
 * SOURCE/SINK FUNCTION ... a primary testing vehicle for USB peripheral
 * controller drivers.
 *
 * This just sinks bulk packets OUT to the peripheral and sources them IN
 * to the host, optionally with specific data patterns for integrity tests.
 * As such it supports basic functionality and load tests.
 *
 * In terms of control messaging, this supports all the standard requests
 * plus two that support control-OUT tests.  If the optional "autoresume"
 * mode is enabled, it provides good functional coverage for the "USBCV"
 * test harness from USB-IF.
 *
 * Note that because this doesn't queue more than one request at a time,
 * some other function must be used to test queueing logic.  The network
 * link (g_ether) is the best overall option for that, since its TX and RX
 * queues are relatively independent, will receive a range of packet sizes,
 * and can often be made to run out completely.  Those issues are important
 * when stress testing peripheral controller drivers.
 *
 *
 * This is currently packaged as a configuration driver, which can't be
 * combined with other functions to make composite devices.  However, it
 * can be combined with other independent configurations.
 */
struct f_rudolf {
	struct usb_function	function;

	struct usb_ep		*in_ep;
	struct usb_ep		*out_ep;

	int			cur_alt;
};

static inline struct f_rudolf *func_to_ss(struct usb_function *f)
{
	return container_of(f, struct f_rudolf, function);
}

enum            // Used for signaling the IN stuff USB-state
{               // Data from the Brick to the HOST
  USB_DATA_IDLE,         //
  USB_DATA_BUSY,         // Ongoing USB request
  USB_DATA_PENDING,      // Data ready for X-fer, but USB busy
  USB_DATA_READY,         // Initial setting
};

int input_state = USB_DATA_IDLE;
struct usb_ep *save_in_ep;
struct usb_request *save_in_req;

/*-------------------------------------------------------------------------*/

static struct usb_interface_descriptor rudolf_intf = {
	.bLength =		sizeof rudolf_intf,
	.bDescriptorType =	USB_DT_INTERFACE,
	.bInterfaceNumber           =     0,
	.bAlternateSetting 	=	0,
	.bNumEndpoints 		=	2,
	.bInterfaceSubClass         =     0,
	.bInterfaceProtocol         =     0,
	.bInterfaceClass =	USB_CLASS_HID,
	/* .iInterface		= DYNAMIC */
};



static struct hid_descriptor hs_hid_rudolf_desc = {
  .bLength                    =     sizeof hs_hid_rudolf_desc,
  .bDescriptorType            =     HID_DT_HID,
  .bcdHID                     =     cpu_to_le16(0x0110),
  .bCountryCode               =     0x00,
  .bNumDescriptors            =     0x01,             // "The one and only"
  .desc[0].bDescriptorType    =     0x22,             // Report Descriptor Type - 0x22 = HID
  .desc[0].wDescriptorLength  =     sizeof hs_hid_report_descriptor,
  /*.desc[0].bDescriptorType  =     DYNAMIC */
  /*.desc[0].wDescriptorLenght=     DYNAMIC */
};

static struct hid_descriptor fs_hid_rudolf_desc = {
  .bLength                    =     sizeof fs_hid_rudolf_desc,
  .bDescriptorType            =     HID_DT_HID,
  .bcdHID                     =     cpu_to_le16(0x0110),
  .bCountryCode               =     0x00,
  .bNumDescriptors            =     0x01,             // "The one and only"
  .desc[0].bDescriptorType    =     0x22,             // Report Descriptor Type - 0x22 = HID
  .desc[0].wDescriptorLength  =     sizeof fs_hid_report_descriptor,

};

/* full speed support: */

static struct usb_endpoint_descriptor rudolf_out_fs_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

  .bEndpointAddress           =     USB_DIR_OUT,
  .bmAttributes               =     USB_ENDPOINT_XFER_INT,
  .wMaxPacketSize             =     cpu_to_le16(64),
  .bInterval                  =     1, /* 1 = 1 mSec POLL rate for FS */
};

static struct usb_endpoint_descriptor rudolf_in_fs_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

  .bEndpointAddress           =     USB_DIR_IN,
  .bmAttributes               =     USB_ENDPOINT_XFER_INT,
  .wMaxPacketSize             =     cpu_to_le16(64),
  .bInterval                  =     1, /* 1 = 1 mSec POLL rate for FS */
};


static struct usb_descriptor_header *fs_rudolf_descs[] = {
  (struct usb_descriptor_header *) &rudolf_intf,
  (struct usb_descriptor_header *) &fs_hid_rudolf_desc,
  (struct usb_descriptor_header *) &rudolf_in_fs_desc,
  (struct usb_descriptor_header *) &rudolf_out_fs_desc,
	NULL,
};

/* high speed support: */

static struct usb_endpoint_descriptor rudolf_in_hs_desc = {
  .bLength                    =     USB_DT_ENDPOINT_SIZE,
  .bDescriptorType            =     USB_DT_ENDPOINT,
  .bEndpointAddress           =     USB_DIR_IN,
  .bmAttributes               =     USB_ENDPOINT_XFER_INT,
  .wMaxPacketSize             =     cpu_to_le16(1024),
  .bInterval                  =     4, /* Calculated as :
                                        * 2^(value-1) * 125uS
                                        * i.e. value 1: 2^(1-1) * 125 uSec = 125 uSec
                                        *      -     4: 2^(4-1) * 125 uSec = 1 mSec
                                        */
};

static struct usb_endpoint_descriptor rudolf_out_hs_desc = {
  .bLength                    =     USB_DT_ENDPOINT_SIZE,
  .bDescriptorType            =     USB_DT_ENDPOINT,
  .bEndpointAddress           =     USB_DIR_OUT,
  .bmAttributes               =     USB_ENDPOINT_XFER_INT,
  .wMaxPacketSize             =     cpu_to_le16(1024),
  .bInterval                  =     4,  /* Calculated as :
                                         * 2^(value-1) * 125uS
                                         * i.e. value 1: 2^(1-1) * 125 uSec = 125 uSec
                                         *      -     4: 2^(4-1) * 125 uSec = 1 mSec
                                         */
};

static struct usb_descriptor_header *hs_rudolf_descs[] = {
  (struct usb_descriptor_header *) &rudolf_intf,
  (struct usb_descriptor_header *) &hs_hid_rudolf_desc,
  (struct usb_descriptor_header *) &rudolf_in_hs_desc,
  (struct usb_descriptor_header *) &rudolf_out_hs_desc,
  NULL,
};


/* function-specific strings: */

static struct usb_string strings_rudolf[] = {
	[0].s = "Xfer data to and from EV3 brick",
	{  }			/* end of list */
};

static struct usb_gadget_strings stringtab_rudolf = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings_rudolf,
};

static struct usb_gadget_strings *rudolf_strings[] = {
	&stringtab_rudolf,
	NULL,
};

/*-------------------------------------------------------------------------*/

struct usb_request *alloc_ep_req(struct usb_ep *ep, int len)
{
	struct usb_request      *req;

	req = usb_ep_alloc_request(ep, GFP_ATOMIC);
	if (req) {
	//	if (len)
	//		req->length = len;
	//	else
			req->length = buflen;
		req->buf = kmalloc(req->length, GFP_ATOMIC);
		if (!req->buf) {
			usb_ep_free_request(ep, req);
			req = NULL;
		}
	}
	return req;
}

void free_ep_req(struct usb_ep *ep, struct usb_request *req)
{
	kfree(req->buf);
	usb_ep_free_request(ep, req);
}

static void disable_ep(struct usb_composite_dev *cdev, struct usb_ep *ep)
{
	int			value;

	if (ep->driver_data) {
		value = usb_ep_disable(ep);
		if (value < 0)
			DBG(cdev, "disable %s --> %d\n",
					ep->name, value);
		ep->driver_data = NULL;
	}
}




void disable_endpoints(struct usb_composite_dev *cdev, struct usb_ep *in, struct usb_ep *out, struct usb_ep *iso_in, struct usb_ep *iso_out)
{
	disable_ep(cdev, in);
	disable_ep(cdev, out);
}





static int
f_rudolf_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct f_rudolf	*ss = func_to_ss(f);
	int	id;
	int ret;

	//printk("liu, usb_function.c---f_rudolf_bind---00\n");
	/* allocate interface ID(s) */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	rudolf_intf.bInterfaceNumber = id;

	//printk("liu, usb_function.c---f_rudolf_bind---11---bInterfaceNumber = %d\n", id);

	/* allocate bulk endpoints */
	ss->in_ep = usb_ep_autoconfig(cdev->gadget, &rudolf_in_fs_desc);
	if (!ss->in_ep) {
	autoconf_fail:
	//	printk("liu, can't autoconfigure in_ep\n");
		ERROR(cdev, "%s: can't autoconfigure on %s\n",
			f->name, cdev->gadget->name);
		return -ENODEV;
	}
	ss->in_ep->driver_data = cdev;	/* claim */




	ss->out_ep = usb_ep_autoconfig(cdev->gadget, &rudolf_out_fs_desc);
	if (!ss->out_ep)
	{
	//	printk("liu, can't autoconfigure out_ep\n");
		goto autoconf_fail;
	}
	ss->out_ep->driver_data = cdev;	/* claim */








	/* support high speed hardware */
	rudolf_in_hs_desc.bEndpointAddress = rudolf_in_fs_desc.bEndpointAddress;
	rudolf_out_hs_desc.bEndpointAddress = rudolf_out_fs_desc.bEndpointAddress;




	//printk("liu, usb_function.c---f_rudolf_bind---22---in and out ep autoconfig OK! in_desc.addr = %x\n", rudolf_in_fs_desc.bEndpointAddress);
	//printk("liu, usb_function.c---f_rudolf_bind---22---in and out ep autoconfig OK! out_desc.addr = %x\n", rudolf_out_fs_desc.bEndpointAddress);

	ret = usb_assign_descriptors(f, fs_rudolf_descs,
			hs_rudolf_descs, NULL);
	if (ret)
		return ret;

	//printk("liu, usb_function.c---f_rudolf_bind---33---usb_assign_descriptors OK!\n");
	/*DBG(cdev, "%s speed %s: IN/%s, OUT/%s, ISO-IN/%s, ISO-OUT/%s\n",
	    (gadget_is_superspeed(c->cdev->gadget) ? "super" :
	     (gadget_is_dualspeed(c->cdev->gadget) ? "dual" : "full")),
			f->name, ss->in_ep->name, ss->out_ep->name,
			ss->iso_in_ep ? ss->iso_in_ep->name : "<none>",
			ss->iso_out_ep ? ss->iso_out_ep->name : "<none>");
	*/

	return 0;
}

static void usb_req_arm(struct usb_ep *ep, struct usb_request *req)
{
	  int status;

	  if (UsbSpeed.Speed == FULL_SPEED)
	   {
	       req->length = 64;
	       req->actual = 64;
	   }
	   else
	   {
	       req->length = 1024;
	       req->actual = 1024;
	   }

	  status = usb_ep_queue(ep, req, GFP_ATOMIC);
	  
	  if (status) {
		    usb_ep_set_halt(ep);
		    /* FIXME recover later ... somehow */
	  }
}

static void
f_rudolf_free_func(struct usb_function *f)
{
	usb_free_all_descriptors(f);
	kfree(func_to_ss(f));
}

/* optionally require specific source/sink data patterns  */
static int read_data_from_host(struct usb_request *req)
{
	  unsigned      i;
	  u8        *buf = req->buf;

	  int from_host_length = 0;  // NO ACCESS LOCKS YET

	  // test for actual length > 0

	  for (i = 0; i < req->actual; i++, buf++)
	  {
		      usb_char_buffer_out[i] = *buf;
		      from_host_length++;
	  }

	  return (from_host_length);
}

static void write_data_to_the_host(struct usb_ep *ep, struct usb_request *req)
{
	  unsigned  i;
	  u8    *buf = req->buf;

	  #ifdef DEBUG
	    printk("WR to HOST req->length = %d\r\n", req->length);
	  #endif

	      #ifdef DEBUG
	        	printk("USB = %d, %d\r\n", usb_char_buffer_in[2], usb_char_buffer_in[3]);
	  #endif

	  for (i = 0; i < req->length; i++)
		    *buf++ = usb_char_buffer_in[i];


	  usb_char_in_length = 0; // Reset and ready
}

static void rudolf_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct usb_composite_dev	*cdev;
	struct f_rudolf			*rudolf = ep->driver_data;
	int				status = req->status;

	/* driver_data will be null if ep has been disabled */
	if (!rudolf)
		return;

	cdev = rudolf->function.config->cdev;

  switch ( status ) {

  case 0:       /* normal completion? */
    if (ep == rudolf->out_ep) // An OUT completion?
    {
      //#define DEBUG
      #ifdef DEBUG
        printk("Rudolf_complete OUT\n");
      #endif

      usb_char_out_length = read_data_from_host(req);
      usb_req_arm(ep, req);
    }
    else  // We have an INPUT request complete
    {
      //#define DEBUG
      #ifdef DEBUG
        printk("Rudolf_complete IN\n");
      #endif

      switch(input_state) // State of Brick data x-fer
      {
        case  USB_DATA_READY:     //should be BUSY or PENDING....

                                  #ifdef DEBUG
                                    printk("IN_IN_IN - READY ?????\n");
                                  #endif

                                  break;

        case  USB_DATA_PENDING:   //
                             //     #define DEBUG
                                  #ifdef DEBUG
                                    printk("IN_IN_IN - PENDING settes to BUSY\n");
                                  #endif

                                  input_state = USB_DATA_BUSY;
                                  write_data_to_the_host(ep, req);
                                  usb_req_arm(ep, req); // new request
                                  break;

        case  USB_DATA_BUSY:      //
                                  #ifdef DEBUG
                                    printk("IN_IN_IN - BUSY settes to READY\n");
                                  #endif
                                  input_state = USB_DATA_READY;
                                  // and relax
                                  break;

        case  USB_DATA_IDLE:      // too lazy
                                  #ifdef DEBUG
                                    printk("IN_IN_IN - IDLE\n");
                                  #endif

                                  break;

        default:                  break;  // hmmm.
      }
              // Reset the buffer size - Ready again
              usb_char_in_length = 0;
    }
      break;

  /* this endpoint is normally active while we're configured */

  case -ESHUTDOWN:      /* disconnect from host */
                        // REMOVED 26102012 (*pUsbSpeed).Speed = FULL_SPEED;
  case -ECONNABORTED:   /* hardware forced ep reset */
  case -ECONNRESET:     /* request dequeued */
  //case -ESHUTDOWN:      /* disconnect from host */
        if (ep == rudolf->out_ep)
          read_data_from_host(req);
        free_ep_req(ep, req);
        return;

  case -EOVERFLOW:    /* buffer overrun on read means that
                         we didn't provide a big enough
                         buffer.
                      */
  default:
//#if 1
  //  DBG(cdev, "%s complete --> %d, %d/%d\n", ep->name,
    //    status, req->actual, req->length);
//#endif
  case -EREMOTEIO:    /* short read */
    break;
  }

}

static int rudolf_start_ep(struct f_rudolf *rudolf, bool is_in)
{
  struct usb_ep   *ep;
  struct usb_request  *req;
  int     status;

  ep = is_in ? rudolf->in_ep : rudolf->out_ep;
  req = alloc_ep_req(ep, 0);
  if (!req)
    return -ENOMEM;

  req->complete = rudolf_complete;

  #ifdef DEBUG
    printk("UsbSpeed.Speed = %d\n\r", UsbSpeed.Speed);
  #endif

  if (UsbSpeed.Speed == FULL_SPEED)
   {
     #ifdef DEBUG
       printk("rudolf_start_ep FULL\n\r");
     #endif

     (*pUsbSpeed).Speed = FULL_SPEED;
     req->length = 64;    // Full speed max buffer size
     req->actual = 64;
   }
   else
   {
     #ifdef DEBUG
       printk("rudolf_start_ep HIGH\n\r");
     #endif

     (*pUsbSpeed).Speed = HIGH_SPEED;
     req->length = 1024;  // High speed max buffer size
     req->actual = 1024;
   }

  if (is_in)
  {
    save_in_ep = ep;
    save_in_req = req;

    #ifdef DEBUG
      printk("req->length = %d ***** Rudolf_Start_Ep_in\n\r", req->length);
    #endif

   // reinit_write_data(ep, req);
    input_state = USB_DATA_BUSY;
  }
  else
  {
    #ifdef DEBUG
      printk("***** Rudolf_Start_Ep_out\n");
    #endif
  }

  status = usb_ep_queue(ep, req, GFP_ATOMIC);

  if (status) {
    struct usb_composite_dev  *cdev;

    cdev = rudolf->function.config->cdev;
    ERROR(cdev, "start %s %s --> %d\n",
        is_in ? "IN" : "OUT",
        ep->name, status);

    free_ep_req(ep, req);
  }

  return status;
}

static void disable_source_sink(struct f_rudolf *ss)
{
	struct usb_composite_dev	*cdev;

	cdev = ss->function.config->cdev;
	disable_endpoints(cdev, ss->in_ep, ss->out_ep, NULL, NULL);
	VDBG(cdev, "%s disabled\n", ss->function.name);
}

static int
enable_source_sink(struct usb_composite_dev *cdev, struct f_rudolf *ss, int alt)
{
	int					result = 0;
	int					speed = cdev->gadget->speed;
	struct usb_ep				*ep;

	//printk("liu, usb_fun.c---enable_source_sink---000\n");
	/* one bulk endpoint writes (sources) zeroes IN (to the host) */
	ep = ss->in_ep;
	result = config_ep_by_speed(cdev->gadget, &(ss->function), ep);
	if (result)
		return result;
	//printk("liu, usb_fun.c---enable_source_sink---001---config_ep_by_speed (in) OK!\n");

	result = usb_ep_enable(ep);
	//printk("liu, usb_fun.c---enable_source_sink---002---usb_ep_enable (in) result = %d\n", result);
	if (result < 0)
		return result;
	//printk("liu, usb_fun.c---enable_source_sink---002---usb_ep_enable (in) OK\n");
	ep->driver_data = ss;

	result = rudolf_start_ep(ss, true);
	if (result < 0) {
fail:
		ep = ss->in_ep;
		usb_ep_disable(ep);
		ep->driver_data = NULL;
		return result;
	}

	//printk("liu, usb_fun.c---enable_source_sink---003---rudolf_start_ep (in) OK\n");



	/* one bulk endpoint reads (sinks) anything OUT (from the host) */
	ep = ss->out_ep;
	result = config_ep_by_speed(cdev->gadget, &(ss->function), ep);
	if (result)
		goto fail;
	//printk("liu, usb_fun.c---enable_source_sink---004---config_ep_by_speed (out) OK!\n");
	result = usb_ep_enable(ep);
	if (result < 0)
		goto fail;
	//printk("liu, usb_fun.c---enable_source_sink---005---usb_ep_enable (out) OK\n");
	ep->driver_data = ss;

	result = rudolf_start_ep(ss, false);
	if (result < 0) {
		ep = ss->out_ep;
		usb_ep_disable(ep);
		ep->driver_data = NULL;
		goto fail;
	}


	//printk("liu, usb_fun.c---enable_source_sink---006---rudolf_start_ep (out) OK\n");

	ss->cur_alt = alt;

	DBG(cdev, "%s enabled, alt intf %d\n", ss->function.name, alt);
	return result;
}





static int f_rudolf_set_alt(struct usb_function *f,
		unsigned intf, unsigned alt)
{
	struct f_rudolf		*ss = func_to_ss(f);
	struct usb_composite_dev	*cdev = f->config->cdev;

	//alt should be zero
	if (ss->in_ep->driver_data)
		disable_source_sink(ss);
	return enable_source_sink(cdev, ss, alt);
}





static int f_rudolf_get_alt(struct usb_function *f, unsigned intf)
{
	struct f_rudolf *ss = func_to_ss(f);

	return ss->cur_alt;
}

static void f_rudolf_disable(struct usb_function *f)
{
	struct f_rudolf *ss = func_to_ss(f);

	disable_source_sink(ss);
}

/*-------------------------------------------------------------------------*/

static int f_rudolf_setup(struct usb_function *f,
		const struct usb_ctrlrequest *ctrl)
{
	struct usb_configuration        *c = f->config;
	struct usb_request	*req = c->cdev->req;
	int			value = -EOPNOTSUPP;
	u16			w_index = le16_to_cpu(ctrl->wIndex);
	u16			w_value = le16_to_cpu(ctrl->wValue);
	u16			w_length = le16_to_cpu(ctrl->wLength);
	u16			length = 0;

	req->length = USB_COMP_EP0_BUFSIZ;

	/* composite driver infrastructure handles everything except
	 * the two control test requests.
	 */
	switch (ctrl->bRequest) {

	/*
	 * These are the same vendor-specific requests supported by
	 * Intel's USB 2.0 compliance test devices.  We exceed that
	 * device spec by allowing multiple-packet requests.
	 *
	 * NOTE:  the Control-OUT data stays in req->buf ... better
	 * would be copying it into a scratch buffer, so that other
	 * requests may safely intervene.
	 */
	case 0x5b:	/* control WRITE test -- fill the buffer */
		if (ctrl->bRequestType != (USB_DIR_OUT|USB_TYPE_VENDOR))
			goto unknown;
		if (w_value || w_index)
			break;
		/* just read that many bytes into the buffer */
		if (w_length > req->length)
			break;
		value = w_length;
		break;
	case 0x5c:	/* control READ test -- return the buffer */
		if (ctrl->bRequestType != (USB_DIR_IN|USB_TYPE_VENDOR))
			goto unknown;
		if (w_value || w_index)
			break;
		/* expect those bytes are still in the buffer; send back */
		if (w_length > req->length)
			break;
		value = w_length;
		break;

	default:
unknown:
		VDBG(c->cdev,
			"unknown control req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
	}




  //HER SKAL HID DESC SENDES!!!
  switch ((ctrl->bRequestType << 8) | ctrl->bRequest) {


  case ((USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_INTERFACE) << 8
      | USB_REQ_GET_DESCRIPTOR):
    switch (w_value >> 8) {
    case HID_DT_REPORT:
      //VDBG(cdev, "USB_REQ_GET_DESCRIPTOR: REPORT\n");
      length = w_length;
      length = min_t(unsigned short, length,
               sizeof hs_hid_report_descriptor);
      memcpy(req->buf, hs_hid_report_descriptor, length);
      value = length;
      goto respond;
      break;

    default:
      //VDBG(cdev, "Unknown decriptor request 0x%x\n",
      //   value >> 8);
      goto stall;
      break;
    }
    break;
  default:
    //VDBG(cdev, "Unknown request 0x%x\n",
    //   ctrl->bRequest);
    goto stall;
    break;
  }

//HERTIL
  /* respond with data transfer or status phase? */
stall:
  return -EOPNOTSUPP;

respond:

	/* respond with data transfer or status phase? */
	if (value >= 0) {
		VDBG(c->cdev, "source/sink req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
		req->zero = 0;
		req->length = value;
		value = usb_ep_queue(c->cdev->gadget->ep0, req, GFP_ATOMIC);
		if (value < 0)
			ERROR(c->cdev, "source/sink response, err %d\n",
					value);
	}

	/* device either stalls (value < 0) or reports success */
	return value;
}

static struct usb_function *source_sink_alloc_func(
		struct usb_function_instance *fi)
{
	struct f_rudolf *ss;
	struct f_ss_opts	*ss_opts;

	ss = kzalloc(sizeof(*ss), GFP_KERNEL);
	if (!ss)
		return NULL;

	ss_opts =  container_of(fi, struct f_ss_opts, func_inst);

	ss->function.name = "rudolf xfer";
	ss->function.bind = f_rudolf_bind;
	ss->function.set_alt = f_rudolf_set_alt;
	ss->function.get_alt = f_rudolf_get_alt;
	ss->function.disable = f_rudolf_disable;
	ss->function.setup = f_rudolf_setup;
	ss->function.strings = rudolf_strings;

	ss->function.free_func = f_rudolf_free_func;

	return &ss->function;
}

static void source_sink_free_instance(struct usb_function_instance *fi)
{
	struct f_ss_opts *ss_opts;

	ss_opts = container_of(fi, struct f_ss_opts, func_inst);
	kfree(ss_opts);
}

static struct usb_function_instance *source_sink_alloc_inst(void)
{
	struct f_ss_opts *ss_opts;

	ss_opts = kzalloc(sizeof(*ss_opts), GFP_KERNEL);
	if (!ss_opts)
		return ERR_PTR(-ENOMEM);
	ss_opts->func_inst.free_func_inst = source_sink_free_instance;
	return &ss_opts->func_inst;
}
DECLARE_USB_FUNCTION(SourceSink, source_sink_alloc_inst,
		source_sink_alloc_func);

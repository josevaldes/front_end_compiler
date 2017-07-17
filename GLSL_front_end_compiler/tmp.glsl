vec2 v2;

bool test(){
	int x = 0;
		int y = 1;
			y = (++x == 1) ? 2 : 0;
				
					bool checkpoint = y == 2 && x == 1;
					return checkpoint;	
							x = 0;
								y = 1;
									y = (x == 0) ? 1 : ++x;
										
											checkpoint = checkpoint && y == 1 && x == 0;
												
													
														x = 0;
															y = 1;
																y = (x != 0) ? ++x : 1;
																	
																		checkpoint = checkpoint && x == 0 && y == 1;
																			
																				
																					x = 0;
																						y = 1;
																							y = (x != 0) ? ++x : x++;
																								
																									checkpoint = checkpoint && x == 1 && y == 0; 
																										
																											return checkpoint;
																											}
